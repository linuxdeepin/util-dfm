// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/semanticsearcher.h>
#include "semanticsearcher_p.h"
#include "semanticquerybuilder.h"
#include "intentparser.h"
#include "semanticruleengine.h"

#include <dfm-search/dsearch_global.h>

#include <QDebug>
#include <QEventLoop>
#include <QTimer>

DFM_SEARCH_BEGIN_NS

SemanticSearcherData::SemanticSearcherData(SemanticSearcher *q_ptr)
    : q(q_ptr)
    , ruleEngine(new SemanticRuleEngine(q))
    , intentParser(new IntentParser(ruleEngine))
    , queryBuilder(new SemanticQueryBuilder())
    , timeoutTimer(new QTimer(q))
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

    cancelled.store(false);
    allResults.clear();
    seenPaths.clear();
    status.store(SearchStatus::Searching);
    Q_EMIT q->statusChanged(SearchStatus::Searching);
    Q_EMIT q->searchStarted();

    // Step 1: Parse natural language into intent
    ParsedIntent intent;
    intentParser->parse(naturalLanguage, intent);

    // Step 2: Build search plan
    const SemanticSearchPlan plan = queryBuilder->build(intent);

    // Step 3: Create and launch search engines in parallel
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

    auto onError = [this](const SearchError &error) {
        qWarning() << "Search error:" << error.message();
        // Don't propagate individual engine errors to caller
        // The other engines may still produce valid results
    };

    // Count how many engines we'll launch
    pendingFinishCount.store(0);

    // Clean up any previous search engines (they have parent q, so Qt deletes them)
    if (fileNameEngine) {
        fileNameEngine->deleteLater();
        fileNameEngine = nullptr;
    }
    if (contentEngine) {
        contentEngine->deleteLater();
        contentEngine = nullptr;
    }
    if (ocrEngine) {
        ocrEngine->deleteLater();
        ocrEngine = nullptr;
    }

    // File name search (always)
    if (Global::isFileNameIndexReadyForSearch()) {
        fileNameEngine = SearchEngine::create(SearchType::FileName, q);
        fileNameEngine->setSearchOptions(plan.fileNameOptions);

        QObject::connect(fileNameEngine, &SearchEngine::resultsFound, q, onResultsFound);
        QObject::connect(fileNameEngine, &SearchEngine::searchFinished, q, onFinished);
        QObject::connect(fileNameEngine, &SearchEngine::errorOccurred, q, onError);

        pendingFinishCount.fetch_add(1);
        fileNameEngine->search(plan.fileNameQuery);
    }

    // Content search
    if (plan.contentQuery.has_value() && plan.contentOptions.has_value()) {
        contentEngine = SearchEngine::create(SearchType::Content, q);
        contentEngine->setSearchOptions(*plan.contentOptions);

        QObject::connect(contentEngine, &SearchEngine::resultsFound, q, onResultsFound);
        QObject::connect(contentEngine, &SearchEngine::searchFinished, q, onFinished);
        QObject::connect(contentEngine, &SearchEngine::errorOccurred, q, onError);

        pendingFinishCount.fetch_add(1);
        contentEngine->search(*plan.contentQuery);
    }

    // OCR search
    if (plan.ocrQuery.has_value() && plan.ocrOptions.has_value()) {
        ocrEngine = SearchEngine::create(SearchType::Ocr, q);
        ocrEngine->setSearchOptions(*plan.ocrOptions);

        QObject::connect(ocrEngine, &SearchEngine::resultsFound, q, onResultsFound);
        QObject::connect(ocrEngine, &SearchEngine::searchFinished, q, onFinished);
        QObject::connect(ocrEngine, &SearchEngine::errorOccurred, q, onError);

        pendingFinishCount.fetch_add(1);
        ocrEngine->search(*plan.ocrQuery);
    }

    // If no engines were launched (e.g., no indexes available)
    if (pendingFinishCount.load() == 0) {
        timeoutTimer->stop();
        status.store(SearchStatus::Finished);
        Q_EMIT q->statusChanged(SearchStatus::Finished);
        Q_EMIT q->searchFinished({});
    } else {
        // Start timeout timer
        if (timeoutSeconds > 0) {
            timeoutTimer->start();
        }
    }
}

void SemanticSearcherData::doCancel()
{
    cancelled.store(true);
    timeoutTimer->stop();

    if (fileNameEngine) {
        fileNameEngine->cancel();
    }
    if (contentEngine) {
        contentEngine->cancel();
    }
    if (ocrEngine) {
        ocrEngine->cancel();
    }
}

// --- SemanticSearcher public API ---

SemanticSearcher::SemanticSearcher(QObject *parent)
    : QObject(parent)
    , d_ptr(new SemanticSearcherData(this))
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
