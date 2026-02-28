// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENTSEARCHAPI_H
#define CONTENTSEARCHAPI_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The ContentOptionsAPI class provides content search specific options
 *
 * This class extends the base SearchOptions with content search specific settings,
 * such as file type filters and content preview length.
 */
class ContentOptionsAPI
{
public:
    /**
     * @brief Constructor
     * @param options The SearchOptions object to operate on
     */
    explicit ContentOptionsAPI(SearchOptions &options);

    /**
     * @brief Set the maximum length for content preview
     * @param length The maximum preview length in characters
     */
    void setMaxPreviewLength(int length);

    /**
     * @brief Get the maximum content preview length
     * @return The maximum preview length in characters
     */
    int maxPreviewLength() const;

    /**
     * @brief Enables or disables HTML highlighting in search results.
     *
     * When enabled, matching keywords in search results will be wrapped in HTML tags
     * (e.g., `<span style="color:red">keyword</span>`) for visual highlighting.
     * Note: Enabling this feature may incur additional processing overhead.
     *
     * @param enable Set to @c true to enable HTML highlighting, @c false to disable.
     */
    void setSearchResultHighlightEnabled(bool enable);

    /**
     * @brief Returns whether HTML highlighting in search results is enabled.
     *
     * @return @c true if search results will include HTML highlighting tags,
     *         @c false otherwise (plaintext results).
     */
    bool isSearchResultHighlightEnabled() const;

    /**
     * @brief Enables or disables full-text content retrieval in search results.
     *
     * When enable, search operations will return the complete file content along with metadata.
     * This provides more detailed results but significantly increases memory usage and processing time.
     *
     * @param enable Set to @c true to retrieve full file contents, @c false to return metadata only.
     */
    void setFullTextRetrievalEnabled(bool enable);

    /**
     * @brief Checks if full-text content retrieval is enabled.
     *
     * @return @c true if search results will include complete file contents,
     *         @c false if only file metadata will be returned.
     */
    bool isFullTextRetrievalEnabled() const;

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

private:
    SearchOptions &m_options;
};

/**
 * @brief The ContentResultAPI class provides content search specific result handling
 *
 * This class extends the base SearchResult with content search specific features,
 * such as highlighted content preview.
 */
class ContentResultAPI
{
public:
    /**
     * @brief Constructor
     * @param result The SearchResult object to operate on
     */
    ContentResultAPI(SearchResult &result);

    /**
     * @brief Get the highlighted content preview
     * @return The highlighted content as QString
     */
    QString highlightedContent() const;

    /**
     * @brief Set the highlighted content preview
     * @param content The highlighted content to set
     */
    void setHighlightedContent(const QString &content);

private:
    SearchResult &m_result;
};
DFM_SEARCH_END_NS

#endif   // CONTENTSEARCHAPI_H
