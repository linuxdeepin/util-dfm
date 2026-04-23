// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TEXTSEARCHAPI_H
#define TEXTSEARCHAPI_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The TextSearchOptionsAPI class provides common text search options
 *
 * This class serves as a base for content and OCR text search options,
 * providing shared functionality for preview length, highlighting, and full-text retrieval.
 */
class TextSearchOptionsAPI
{
public:
    /**
     * @brief Constructor
     * @param options The SearchOptions object to operate on
     */
    explicit TextSearchOptionsAPI(SearchOptions &options);

    // ==================== Preview and Highlight Settings ====================

    /**
     * @brief Sets the maximum length for content preview in search results.
     * @param length The maximum preview length in characters.
     */
    void setMaxPreviewLength(int length);

    /**
     * @brief Gets the maximum length for content preview in search results.
     * @return The maximum preview length in characters.
     */
    int maxPreviewLength() const;

    /**
     * @brief Enables or disables HTML highlighting in search results.
     *
     * When enabled, matching keywords in search results will be wrapped in HTML tags
     * (e.g., `<b>keyword</b>`) for visual highlighting.
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
     * When enabled, search operations will return the complete text content along with metadata.
     * This provides more detailed results but significantly increases memory usage and processing time.
     *
     * @param enable Set to @c true to retrieve full text contents, @c false to return metadata only.
     */
    void setFullTextRetrievalEnabled(bool enable);

    /**
     * @brief Returns whether full-text content retrieval is enabled.
     *
     * @return @c true if full-text content retrieval is enabled, @c false otherwise.
     */
    bool isFullTextRetrievalEnabled() const;

protected:
    SearchOptions &m_options;
};

/**
 * @brief The TextSearchResultAPI class provides common text search result handling
 *
 * This class serves as a base for content and OCR text search results,
 * providing shared functionality for highlighted content and file metadata.
 */
class TextSearchResultAPI
{
public:
    /**
     * @brief Constructor
     * @param result The SearchResult object to operate on
     */
    explicit TextSearchResultAPI(SearchResult &result);

    // ==================== Highlighted Content ====================

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

    // ==================== Extended Attributes ====================

    /**
     * @brief Get the file name (without path)
     * @return The file name
     */
    QString filename() const;

    /**
     * @brief Set the file name
     * @param name The file name to set
     */
    void setFilename(const QString &name);

    /**
     * @brief Check if the file is hidden
     * @return true if the file is hidden, false otherwise
     */
    bool isHidden() const;

    /**
     * @brief Set whether the file is hidden
     * @param hidden true if the file is hidden, false otherwise
     */
    void setIsHidden(bool hidden);

    // ==================== Modification Time ====================

    /**
     * @brief Set the modification time timestamp
     * @param timestamp Unix timestamp in seconds
     */
    void setModifyTimestamp(qint64 timestamp);

    /**
     * @brief Get the modification time timestamp
     * @return Unix timestamp in seconds, 0 if not set
     */
    qint64 modifyTimestamp() const;

    /**
     * @brief Get the modification time as a formatted string
     * @return Formatted time string (yyyy-MM-dd HH:mm:ss)
     */
    QString modifyTimeString() const;

    // ==================== Birth/Creation Time ====================

    /**
     * @brief Set the birth/creation time timestamp
     * @param timestamp Unix timestamp in seconds
     */
    void setBirthTimestamp(qint64 timestamp);

    /**
     * @brief Get the birth/creation time timestamp
     * @return Unix timestamp in seconds, 0 if not set
     */
    qint64 birthTimestamp() const;

    /**
     * @brief Get the birth/creation time as a formatted string
     * @return Formatted time string (yyyy-MM-dd HH:mm:ss)
     */
    QString birthTimeString() const;

protected:
    SearchResult &m_result;
};

DFM_SEARCH_END_NS

#endif   // TEXTSEARCHAPI_H
