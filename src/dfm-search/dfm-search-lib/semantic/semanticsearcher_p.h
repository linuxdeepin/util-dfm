// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTICSEARCHER_P_H
#define SEMANTICSEARCHER_P_H

#include <QSet>
#include <QTimer>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchengine.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;
class IntentParser;
class SemanticQueryBuilder;
class SemanticSearchPlan;

class SemanticSearcherData
{
public:
    explicit SemanticSearcherData(SemanticSearcher *q);
    ~SemanticSearcherData();

    void doSearch(const QString &naturalLanguage);
    void doCancel();

    SemanticSearcher *q = nullptr;

    // State
    std::atomic<SearchStatus> status{SearchStatus::Ready};
    std::atomic<bool> cancelled{false};
    int timeoutSeconds = 60;

    // Core components (owned)
    SemanticRuleEngine *ruleEngine = nullptr;
    IntentParser *intentParser = nullptr;
    SemanticQueryBuilder *queryBuilder = nullptr;

    // Sub-engines (owned per search)
    SearchEngine *fileNameEngine = nullptr;
    SearchEngine *contentEngine = nullptr;
    SearchEngine *ocrEngine = nullptr;
    std::atomic<int> pendingFinishCount{0};

    // Result collection
    SearchResultList allResults;
    QSet<QString> seenPaths;

    // Timeout
    QTimer *timeoutTimer = nullptr;
};

DFM_SEARCH_END_NS

#endif   // SEMANTICSEARCHER_P_H
