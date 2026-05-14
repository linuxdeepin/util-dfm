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

void SemanticSearcherData::doSearch(const QString &naturalLanguage)
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
    Q_EMIT q->searchStarted();

    // Step 2: Parse natural language into intent
    ParsedIntent intent;
    intentParser->parse(naturalLanguage, intent);

    // Step 3: Build search plan
    const SemanticSearchPlan plan = queryBuilder->build(intent);

    // Step 4: Determine search directories
    QStringList dirs = plan.searchDirectories.isEmpty()
            ? QStringList { QDir::homePath() }
            : plan.searchDirectories;

    // Step 5: Set up signal/slot handlers
    auto onResultsFound = [this](const SearchResultList &results) {
        SearchResultList newResults;
        for (const SearchResult &r : results) {
            if (!seenPaths.contains(r.path())) {
                seenPaths.insert(r.path());
                newResults.append(r);
            }
        }
        if (!newResults.isEmpty()) {
            allResults.append(newResults);
            Q_EMIT q->resultsFound(newResults);
        }
    };

    auto onFinished = [this](const SearchResultList &) {
        if (pendingFinishCount.fetch_sub(1) == 1) {
            // All engines finished
            timeoutTimer->stop();
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
        return opts;
    };

    // Step 8: Launch up to 3 engines (FileName, Content, OCR)
    // TimeField::Both is no longer expanded here; it is handled by the Lucene strategy layer.
    // Multiple directories are passed via setSearchPaths().

    // File name search (always, if index is ready)
    if (Global::isFileNameIndexReadyForSearch()) {
        SearchOptions fnameOpts = prepareOptions(plan.fileNameOptions);
        createAndLaunchEngine(SearchType::FileName, plan.fileNameQuery,
                              fnameOpts, onResultsFound, onFinished, onError);
    }

    // Content search
    if (plan.contentQuery.has_value() && plan.contentOptions.has_value()) {
        SearchOptions contentOpts = prepareOptions(*plan.contentOptions);
        createAndLaunchEngine(SearchType::Content, *plan.contentQuery,
                              contentOpts, onResultsFound, onFinished, onError);
    }

    // OCR search
    if (plan.ocrQuery.has_value() && plan.ocrOptions.has_value()) {
        SearchOptions ocrOpts = prepareOptions(*plan.ocrOptions);
        createAndLaunchEngine(SearchType::Ocr, *plan.ocrQuery,
                              ocrOpts, onResultsFound, onFinished, onError);
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
        std::function<void(const SearchResultList &)> onResultsFound,
        std::function<void(const SearchResultList &)> onFinished,
        std::function<void(const SearchError &)> onError)
{
    SearchEngine *engine = SearchEngine::create(type, q);
    engine->setSearchOptions(options);

    QObject::connect(engine, &SearchEngine::resultsFound, q, onResultsFound);
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

    d_ptr->doSearch(naturalLanguage);
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
            || !intent.searchDirectories.isEmpty();
}

void SemanticSearcher::cancel()
{
    d_ptr->doCancel();
}

SearchResultExpected SemanticSearcher::searchSync(const QString &naturalLanguage)
{
    if (d_ptr->status.load() == SearchStatus::Searching) {
        qWarning() << "Search already in progress";
        return Dtk::Core::DUnexpected<SearchError>(SearchError(SearchErrorCode::InvalidQuery));
    }

    if (naturalLanguage.trimmed().isEmpty()) {
        return Dtk::Core::DUnexpected<SearchError>(SearchError(SearchErrorCode::InvalidQuery));
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
    d_ptr->doSearch(naturalLanguage);

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
