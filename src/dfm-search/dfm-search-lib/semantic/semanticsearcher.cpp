// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/semanticsearcher.h>
#include "semanticsearcher_p.h"
#include "semanticquerybuilder.h"
#include "intentparser.h"
#include "semanticruleengine.h"

#include <dfm-search/dsearch_global.h>
#include <dfm-search/semantic_types.h>

#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QTimer>

DFM_SEARCH_BEGIN_NS

namespace {

bool hasKeywordRuleSpan(const ParsedIntent &intent)
{
    for (const MatchSpan &span : intent.consumedSpans()) {
        if (span.ruleId().startsWith(QStringLiteral("keyword_"))) {
            return true;
        }
    }
    return false;
}

bool hasTargetRuleSpan(const ParsedIntent &intent)
{
    for (const MatchSpan &span : intent.consumedSpans()) {
        if (span.ruleId().startsWith(QStringLiteral("target_"))) {
            return true;
        }
    }
    return false;
}

// 计算语义维度个数。任何单维度单独都不算语义查询（"下载"、"隐藏的"、"打开过的"、
// "图片"、"去年"、"大于10M" 都只是单维度），必须至少 2 个维度组合才表达完整搜索意图。
// 维度：location / hiddenOnly / recentOnly / time / size / fileType / keyword / target
int countSemanticDimensions(const ParsedIntent &intent)
{
    int count = 0;
    if (!intent.searchDirectories().isEmpty()) ++count;   // location
    if (intent.hiddenOnly()) ++count;                     // hiddenOnly 属性
    if (intent.recentOnly()) ++count;                     // recentOnly 属性
    if (intent.timeConstraint().isValid()) ++count;       // time 约束
    if (intent.sizeConstraint().isValid()) ++count;       // size 约束
    if (!intent.fileExtensions().isEmpty()) ++count;      // fileType
    if (!intent.keywords().isEmpty()) ++count;            // keyword (unconsumed text)
    if (hasTargetRuleSpan(intent)) ++count;               // target 名词 (文件/文件名/文件夹)

    // 当 specific filetype 与 general filetype 同时命中（如 "PPT文档"、"pdf文档"），
    // general 类型词（文档/表格/幻灯片）充当目标名词，贡献额外维度。
    // 单独的 general filetype（"文档"）或单独的 specific filetype（"PPT"）不触发，
    // 仍为单维度，由 guardNonSemantic 拦下。
    bool hasSpecificFt = false;
    bool hasGeneralFt = false;
    for (const MatchSpan &span : intent.consumedSpans()) {
        const QString rid = span.ruleId();
        if (rid.startsWith(QLatin1String("filetype_"))) {
            if (rid.endsWith(QLatin1String("_general")))
                hasGeneralFt = true;
            else
                hasSpecificFt = true;
        }
    }
    if (hasSpecificFt && hasGeneralFt) ++count;

    return count;
}

bool isSemanticIntent(const ParsedIntent &intent)
{
    // 结构化关键字规则 (keyword_* span) 本身已表达明确搜索意图，单独成立
    // (如 "包含测试的文件"、"文件名包含报告的文档")
    if (hasKeywordRuleSpan(intent)) {
        return true;
    }

    // 至少 2 个语义维度组合才算完整语义查询。
    // 单维度（纯位置/纯属性/纯时间/纯大小/纯文件类型）不构成可执行的搜索意图，
    // 由 guardNonSemantic 拦下报 InvalidQuery，避免进入 doSearch 后空关键字报错。
    return countSemanticDimensions(intent) >= 2;
}

}   // namespace

SemanticSearcherData::SemanticSearcherData(SemanticSearcher *q_ptr)
    : q(q_ptr), ruleEngine(new SemanticRuleEngine(q)), intentParser(new IntentParser(ruleEngine)), queryBuilder(new SemanticQueryBuilder()), timeoutTimer(new QTimer(q))
{
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(timeoutSeconds * 1000);

    QObject::connect(timeoutTimer, &QTimer::timeout, q, [this]() {
        qWarning() << "Semantic search timed out after" << timeoutSeconds << "seconds";
        doCancel();
    });

    // Load rules
    if (!ruleEngine->loadRules()) {
        qWarning() << "Failed to load semantic rules";
    }
}

SemanticSearcherData::~SemanticSearcherData()
{
    doCancel();
}

SearchError SemanticSearcherData::guardNonSemantic(const QString &input)
{
    if (q->isSemanticQuery(input))
        return SearchError();   // valid semantic query

    qWarning() << "Non-semantic query:" << input
               << "- Use FileName/Content search instead";
    return SearchError(SearchErrorCode::InvalidQuery);
}

void SemanticSearcherData::doSearch(const QString &naturalLanguage, const QStringList &searchDirectories)
{
    if (naturalLanguage.trimmed().isEmpty()) {
        Q_EMIT q->errorOccurred(SearchError(SearchErrorCode::InvalidQuery));
        return;
    }

    // Step 1: Validate + reset state
    cancelled.store(false);
    allResults.clear();
    seenPaths.clear();
    status.store(SearchStatus::Searching);
    Q_EMIT q->statusChanged(SearchStatus::Searching);

    // Step 2: Parse natural language into intent (before searchStarted
    // so that intentParsed listeners have the data when searchStarted fires)
    ParsedIntent intent;
    intentParser->parse(naturalLanguage, intent);
    Q_EMIT q->intentParsed(intent);

    Q_EMIT q->searchStarted();

    // Step 3: Build search plan
    const SemanticSearchPlan plan = queryBuilder->build(intent);

    // Step 4: Determine search directories
    // Priority: NLP-parsed directories > caller-specified directories > home directory
    // NLP 解析出的路径是用户意图的最直接表达（如"在文档里找xxx"），
    // 应优先于调用者传入的默认路径。
    QStringList dirs;
    if (!plan.searchDirectories.isEmpty()) {
        dirs = plan.searchDirectories;
    } else if (!searchDirectories.isEmpty()) {
        dirs = searchDirectories;
    } else {
        dirs = QStringList { QDir::homePath() };
    }

    // Step 5: Set up signal/slot handlers
    auto onFinished = [this](const SearchResultList &results) {
        // Collect and deduplicate results from each engine's final result list
        for (const SearchResult &r : results) {
            if (!seenPaths.contains(r.path())) {
                seenPaths.insert(r.path());
                allResults.append(r);
            }
        }

        if (pendingFinishCount.fetch_sub(1) == 1) {
            // All engines finished
            timeoutTimer->stop();

            // Truncate final deduplicated results to maxResults
            if (maxResults > 0 && allResults.size() > maxResults) {
                allResults = allResults.mid(0, maxResults);
            }

            if (cancelled.load()) {
                status.store(SearchStatus::Cancelled);
                Q_EMIT q->statusChanged(SearchStatus::Cancelled);
                Q_EMIT q->searchCancelled();
            } else {
                status.store(SearchStatus::Finished);
                Q_EMIT q->statusChanged(SearchStatus::Finished);
                Q_EMIT q->searchFinished(allResults);
            }
        }
    };

    // 引擎错误也必须触发计数归零检查，否则上层会一直阻塞到 60s 超时。
    // 校验失败（如 KeywordIsEmpty）会让 GenericSearchEngine::search 在
    // emit errorOccurred 后直接 return，不会 emit searchFinished ——
    // 此时必须由 onError 兜底减计数，把错误视为引擎完成。
    auto onError = [this, onFinished](const SearchError &error) {
        qWarning() << "Search error:" << error.message();
        // Don't propagate individual engine errors to caller
        // The other engines may still produce valid results
        // 但仍需减计数，否则 pendingFinishCount 永不到 0
        onFinished({});
    };

    // Step 6: Clean up any previous search engines
    pendingFinishCount.store(0);
    for (SearchEngine *e : engines) {
        e->deleteLater();
    }
    engines.clear();

    // Step 7: Helper to prepare options with multi-path and hidden settings
    auto prepareOptions = [&dirs, &plan](const SearchOptions &baseOpts) -> SearchOptions {
        SearchOptions opts = baseOpts;
        opts.setSearchPaths(dirs);
        if (plan.includeHidden) {
            opts.setIncludeHidden(true);
        }
        if (plan.hiddenOnly) {
            opts.setHiddenOnly(true);
        }
        return opts;
    };

    // Apply caller-level options
    auto applyCallerOptions = [this](SearchOptions &opts) {
        if (detailedResultsEnabled) {
            opts.setDetailedResultsEnabled(true);
        }
        if (maxResults > 0) {
            opts.setMaxResults(maxResults);
        }
        if (!excludedPaths.isEmpty()) {
            opts.setSearchExcludedPaths(excludedPaths);
        }
    };

    // Step 8: Launch engines based on plan
    // TimeField::Both is no longer expanded here; it is handled by the Lucene strategy layer.
    // Multiple directories are passed via setSearchPaths().

    // ── Recently-used files: DBus data source, exclusive path ──
    // recentOnly 抑制所有 index-based 引擎（数据不在索引中）。
    if (plan.recentOnly && plan.recentQuery.has_value() && plan.recentOptions.has_value()) {
        SearchOptions recentOpts = *plan.recentOptions;
        applyCallerOptions(recentOpts);
        createAndLaunchEngine(SearchType::Recent, *plan.recentQuery,
                              recentOpts, onFinished, onError);
    } else {
        // ── Index-based engines: FileName, Content, OCR ──

        // File name search (always, if index is ready)
        if (Global::isFileNameIndexReadyForSearch()) {
            SearchOptions fnameOpts = prepareOptions(plan.fileNameOptions);
            applyCallerOptions(fnameOpts);
            createAndLaunchEngine(SearchType::FileName, plan.fileNameQuery,
                                  fnameOpts, onFinished, onError);
        }

        // Content search
        if (plan.contentQuery.has_value() && plan.contentOptions.has_value()) {
            SearchOptions contentOpts = prepareOptions(*plan.contentOptions);
            applyCallerOptions(contentOpts);
            createAndLaunchEngine(SearchType::Content, *plan.contentQuery,
                                  contentOpts, onFinished, onError);
        }

        // OCR search
        if (plan.ocrQuery.has_value() && plan.ocrOptions.has_value()) {
            SearchOptions ocrOpts = prepareOptions(*plan.ocrOptions);
            applyCallerOptions(ocrOpts);
            createAndLaunchEngine(SearchType::Ocr, *plan.ocrQuery,
                                  ocrOpts, onFinished, onError);
        }
    }   // end of else (index-based engines)

    // Step 9: Handle no-engine case
    if (pendingFinishCount.load() == 0) {
        timeoutTimer->stop();
        status.store(SearchStatus::Finished);
        Q_EMIT q->statusChanged(SearchStatus::Finished);
        Q_EMIT q->searchFinished({});
    } else {
        if (timeoutSeconds > 0) {
            timeoutTimer->start();
        }
    }
}

void SemanticSearcherData::createAndLaunchEngine(
        SearchType type,
        const SearchQuery &query,
        const SearchOptions &options,
        std::function<void(const SearchResultList &)> onFinished,
        std::function<void(const SearchError &)> onError)
{
    SearchEngine *engine = SearchEngine::create(type, q);
    engine->setSearchOptions(options);

    QObject::connect(engine, &SearchEngine::searchFinished, q, onFinished);
    QObject::connect(engine, &SearchEngine::errorOccurred, q, onError);

    engines.append(engine);
    pendingFinishCount.fetch_add(1);
    engine->search(query);
}

void SemanticSearcherData::doCancel()
{
    cancelled.store(true);
    timeoutTimer->stop();

    for (SearchEngine *e : engines) {
        e->cancel();
    }
}

// --- SemanticSearcher public API ---

SemanticSearcher::SemanticSearcher(QObject *parent)
    : QObject(parent), d_ptr(new SemanticSearcherData(this))
{
}

SemanticSearcher::~SemanticSearcher() = default;

SearchStatus SemanticSearcher::status() const
{
    return d_ptr->status.load();
}

void SemanticSearcher::setSearchTimeout(int seconds)
{
    d_ptr->timeoutSeconds = seconds;
    d_ptr->timeoutTimer->setInterval(seconds * 1000);
}

int SemanticSearcher::searchTimeout() const
{
    return d_ptr->timeoutSeconds;
}

void SemanticSearcher::search(const QString &naturalLanguage)
{
    if (d_ptr->status.load() == SearchStatus::Searching) {
        qWarning() << "Search already in progress";
        return;
    }

    if (const auto err = d_ptr->guardNonSemantic(naturalLanguage); err.isError()) {
        Q_EMIT errorOccurred(err);
        return;
    }

    d_ptr->doSearch(naturalLanguage, {});
}

void SemanticSearcher::search(const QString &naturalLanguage, const QStringList &searchDirectories)
{
    if (d_ptr->status.load() == SearchStatus::Searching) {
        qWarning() << "Search already in progress";
        return;
    }

    if (const auto err = d_ptr->guardNonSemantic(naturalLanguage); err.isError()) {
        Q_EMIT errorOccurred(err);
        return;
    }

    d_ptr->doSearch(naturalLanguage, searchDirectories);
}

ParsedIntent SemanticSearcher::parseIntent(const QString &input) const
{
    ParsedIntent intent;
    if (input.trimmed().isEmpty()) {
        return intent;   // default-constructed: all constraints invalid/empty
    }

    d_ptr->intentParser->parse(input, intent);
    return intent;
}

bool SemanticSearcher::isSemanticQuery(const QString &input) const
{
    return isSemanticIntent(parseIntent(input));
}

bool SemanticSearcher::isSemanticQuery(const ParsedIntent &intent) const
{
    return isSemanticIntent(intent);
}

void SemanticSearcher::cancel()
{
    d_ptr->doCancel();
}

void SemanticSearcher::setDetailedResultsEnabled(bool enable)
{
    d_ptr->detailedResultsEnabled = enable;
}

bool SemanticSearcher::isDetailedResultsEnabled() const
{
    return d_ptr->detailedResultsEnabled;
}

void SemanticSearcher::setMaxResults(int count)
{
    d_ptr->maxResults = count;
}

int SemanticSearcher::maxResults() const
{
    return d_ptr->maxResults;
}

void SemanticSearcher::setSearchExcludedPaths(const QStringList &paths)
{
    d_ptr->excludedPaths = paths;
}

SearchResultExpected SemanticSearcher::searchSync(const QString &naturalLanguage)
{
    return searchSync(naturalLanguage, {});
}

SearchResultExpected SemanticSearcher::searchSync(const QString &naturalLanguage, const QStringList &searchDirectories)
{
    if (d_ptr->status.load() == SearchStatus::Searching) {
        qWarning() << "Search already in progress";
        return Dtk::Core::DUnexpected<SearchError>(SearchError(SearchErrorCode::InvalidQuery));
    }

    if (naturalLanguage.trimmed().isEmpty()) {
        return Dtk::Core::DUnexpected<SearchError>(SearchError(SearchErrorCode::InvalidQuery));
    }

    if (const auto err = d_ptr->guardNonSemantic(naturalLanguage); err.isError()) {
        return Dtk::Core::DUnexpected<SearchError>(err);
    }

    SearchResultList results;
    bool hasError = false;
    SearchError lastError;
    bool cancelled = false;
    bool done = false;

    QEventLoop eventLoop;

    // Use a shared guard flag so late-arriving signals after eventLoop exits are harmless.
    // The internal doSearch timeout mechanism is relied upon for actual cancellation.
    QObject::connect(this, &SemanticSearcher::searchFinished, this,
                     [&](const SearchResultList &r) {
                         if (!done) {
                             results = r;
                             done = true;
                             eventLoop.quit();
                         }
                     });

    QObject::connect(this, &SemanticSearcher::searchCancelled, this,
                     [&]() {
                         if (!done) {
                             cancelled = true;
                             done = true;
                             eventLoop.quit();
                         }
                     });

    QObject::connect(this, &SemanticSearcher::errorOccurred, this,
                     [&](const SearchError &error) {
                         if (!done) {
                             hasError = true;
                             lastError = error;
                             done = true;
                             eventLoop.quit();
                         }
                     });

    // Start the async search (uses internal timeout mechanism)
    d_ptr->doSearch(naturalLanguage, searchDirectories);

    // Block until completion, cancellation, or error
    eventLoop.exec();

    if (cancelled) {
        return results;
    }

    if (hasError) {
        return Dtk::Core::DUnexpected<SearchError>(lastError);
    }

    return results;
}

DFM_SEARCH_END_NS
