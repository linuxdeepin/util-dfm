// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENTSEARCHAPI_H
#define CONTENTSEARCHAPI_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>
#include <dfm-search/textsearchapi.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The ContentOptionsAPI class provides content search specific options
 *
 * This class extends TextSearchOptionsAPI with content search specific settings.
 */
class ContentOptionsAPI : public TextSearchOptionsAPI
{
public:
    /**
     * @brief Constructor
     * @param options The SearchOptions object to operate on
     */
    explicit ContentOptionsAPI(SearchOptions &options);

    /**
     * @brief Sets whether the extended AND search behavior across 'contents' and 'filename' fields is enabled.
     * @param enabled True to enable the feature, false to disable it.
     * @see isFilenameContentMixedAndSearchEnabled() for a detailed description of the behavior.
     */
    void setFilenameContentMixedAndSearchEnabled(bool enabled);

    /**
     * @brief Checks if the extended AND search behavior across 'contents' and 'filename' fields is enabled.
     *
     * When enabled (returns true), boolean AND queries will search for terms such that:
     * 1. All terms must be present, potentially distributed between the 'contents' and 'filename' fields.
     *    (e.g., termA in 'contents', termB in 'filename').
     * 2. A match is explicitly excluded if all search terms are found *only* within the 'filename' field.
     *    (e.g., termA in 'filename', termB in 'filename' -- this specific case is excluded).
     * 3. Matches where all terms are in 'contents', or mixed between 'contents' and 'filename' (as in point 1), are included.
     *
     * If this option is disabled (returns false), or for boolean OR queries,
     * the boolean search will be performed exclusively on the 'contents' field, following the original logic.
     *
     * @return True if the filename-content mixed AND search is enabled, false otherwise.
     */
    bool isFilenameContentMixedAndSearchEnabled() const;
};

/**
 * @brief The ContentResultAPI class provides content search specific result handling
 *
 * This class extends TextSearchResultAPI for content search results.
 */
class ContentResultAPI : public TextSearchResultAPI
{
public:
    /**
     * @brief Constructor
     * @param result The SearchResult object to operate on
     */
    explicit ContentResultAPI(SearchResult &result);
};

DFM_SEARCH_END_NS

#endif   // CONTENTSEARCHAPI_H
