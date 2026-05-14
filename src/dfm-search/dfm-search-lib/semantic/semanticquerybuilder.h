// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTICQUERYBUILDER_H
#define SEMANTICQUERYBUILDER_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/semantic_types.h>

#include <optional>

DFM_SEARCH_BEGIN_NS

/**
 * @brief Search plan for all three search paths.
 */
struct SemanticSearchPlan {
    SearchQuery fileNameQuery;
    SearchOptions fileNameOptions;
    std::optional<SearchQuery> contentQuery;
    std::optional<SearchOptions> contentOptions;
    std::optional<SearchQuery> ocrQuery;
    std::optional<SearchOptions> ocrOptions;
    TimeField timeField = TimeField::ModifyTime;   // BirthTime, ModifyTime, or Both
    QStringList searchDirectories;   // Empty = use default homePath
    bool includeHidden = false;      // For trash directory
};

/**
 * @brief Converts a ParsedIntent into concrete SearchQuery + SearchOptions for each path.
 */
class SemanticQueryBuilder
{
public:
    SemanticQueryBuilder();
    ~SemanticQueryBuilder();

    /**
     * @brief Build a search plan from parsed intent.
     * @param intent The parsed intent
     * @return A search plan with queries and options for each search path
     */
    SemanticSearchPlan build(const ParsedIntent &intent);

private:
    TimeRangeFilter buildTimeRangeFilter(const TimeConstraint &tc) const;
    SizeRangeFilter buildSizeRangeFilter(const SizeConstraint &sc) const;
    SearchOptions buildBaseOptions(const TimeConstraint &tc, const SizeConstraint &sc) const;
};

DFM_SEARCH_END_NS

#endif   // SEMANTICQUERYBUILDER_H
