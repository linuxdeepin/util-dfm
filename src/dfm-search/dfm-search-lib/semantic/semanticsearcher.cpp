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
    // Priority: caller-specified directories > NLP-parsed directories > home directory
    QStringList dirs;
    if (!searchDirectories.isEmpty()) {
        dirs = searchDirectories;
    } else if (!plan.searchDirectories.isEmpty()) {
        dirs = plan.searchDirectories;
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

    auto onError = [](const SearchError &error) {
        qWarning() << "Search error:" << error.message();
        // Don't propagate individual engine errors to caller
        // The other engines may still produce valid results
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
    };

    // Step 8: Launch up to 3 engines (FileName, Content, OCR)
    // TimeField::Both is no longer expanded here; it is handled by the Lucene strategy layer.
    // Multiple directories are passed via setSearchPaths().

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

bool SemanticSearcher::isSemanticQuery(const QString &input) const
{
    if (input.trimmed().isEmpty()) {
        return false;
    }

    ParsedIntent intent;
    d_ptr->intentParser->parse(input, intent);

    return intent.timeConstraint.isValid()
            || intent.sizeConstraint.isValid()
            || !intent.fileExtensions.isEmpty()
            || !intent.searchDirectories.isEmpty()
            || intent.includeHidden
            || intent.hiddenOnly
            || !intent.consumedSpans.isEmpty();
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
