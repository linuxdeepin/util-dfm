// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTICSEARCHER_P_H
#define SEMANTICSEARCHER_P_H

#include <QSet>
#include <QTimer>

#include <functional>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchengine.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;
class IntentParser;
class SemanticQueryBuilder;
class SemanticSearchPlan;
class SemanticSearcher;

class SemanticSearcherData
{
public:
    explicit SemanticSearcherData(SemanticSearcher *q);
    ~SemanticSearcherData();

    void doSearch(const QString &naturalLanguage, const QStringList &searchDirectories);
    void doCancel();

    /**
     * @brief Create, configure, and launch a search engine
     *
     * Creates a SearchEngine of the given type, sets its options, connects
     * signal/slot handlers, appends it to the engines list, increments the
     * pending finish counter, and starts the search.
     *
     * @param type The search engine type (FileName, Content, or Ocr)
     * @param query The search query to execute
     * @param options The search options (including multi-path and time filter)
     * @param onFinished Callback for engine completion and result collection
     * @param onError Callback for error handling
     */
    void createAndLaunchEngine(SearchType type,
                               const SearchQuery &query,
                               const SearchOptions &options,
                               std::function<void(const SearchResultList &)> onFinished,
                               std::function<void(const SearchError &)> onError);

    SemanticSearcher *q = nullptr;

    // State
    std::atomic<SearchStatus> status { SearchStatus::Ready };
    std::atomic<bool> cancelled { false };
    int timeoutSeconds = 60;

    // Core components (owned)
    SemanticRuleEngine *ruleEngine = nullptr;
    IntentParser *intentParser = nullptr;
    SemanticQueryBuilder *queryBuilder = nullptr;

    // Sub-engines (owned per search, parented to q for auto-cleanup)
    QList<SearchEngine *> engines;
    std::atomic<int> pendingFinishCount { 0 };

    // Result collection
    SearchResultList allResults;
    QSet<QString> seenPaths;

    // Timeout
    QTimer *timeoutTimer = nullptr;

    // Options forwarded from caller
    bool detailedResultsEnabled = false;
    int maxResults = 0;   // 0 = unlimited
};

DFM_SEARCH_END_NS

#endif   // SEMANTICSEARCHER_P_H
