// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OCRTEXTSEARCHAPI_H
#define OCRTEXTSEARCHAPI_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The OcrTextOptionsAPI class provides OCR text search specific options
 *
 * This class extends the base SearchOptions with OCR text search specific settings.
 */
class OcrTextOptionsAPI
{
public:
    /**
     * @brief Constructor
     * @param options The SearchOptions object to operate on
     */
    explicit OcrTextOptionsAPI(SearchOptions &options);

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
     * When enabled, search operations will return the complete OCR text content along with metadata.
     * This provides more detailed results but significantly increases memory usage and processing time.
     *
     * @param enable Set to @c true to retrieve full OCR text contents, @c false to return metadata only.
     */
    void setFullTextRetrievalEnabled(bool enable);

    /**
     * @brief Returns whether full-text content retrieval is enabled.
     *
     * @return @c true if full-text content retrieval is enabled, @c false otherwise.
     */
    bool isFullTextRetrievalEnabled() const;

    // ==================== Mixed Search Settings ====================

    /**
     * @brief Sets whether the extended AND search behavior across 'ocr_contents' and 'filename' fields is enabled.
     * @param enabled True to enable the feature, false to disable it.
     * @see isFilenameOcrContentMixedAndSearchEnabled() for a detailed description of the behavior.
     */
    void setFilenameOcrContentMixedAndSearchEnabled(bool enabled);

    /**
     * @brief Checks if the extended AND search behavior across 'ocr_contents' and 'filename' fields is enabled.
     *
     * When enabled (returns true), boolean AND queries will search for terms such that:
     * 1. All terms must be present, potentially distributed between the 'ocr_contents' and 'filename' fields.
     *    (e.g., termA in 'ocr_contents', termB in 'filename').
     * 2. A match is explicitly excluded if all search terms are found *only* within the 'filename' field.
     *    (e.g., termA in 'filename', termB in 'filename' -- this specific case is excluded).
     * 3. Matches where all terms are in 'ocr_contents', or mixed between 'ocr_contents' and 'filename' (as in point 1), are included.
     *
     * If this option is disabled (returns false), or for boolean OR queries,
     * the boolean search will be performed exclusively on the 'ocr_contents' field, following the original logic.
     *
     * @return True if the filename-OCR content mixed AND search is enabled, false otherwise.
     */
    bool isFilenameOcrContentMixedAndSearchEnabled() const;

private:
    SearchOptions &m_options;
};

/**
 * @brief The OcrTextResultAPI class provides OCR text search specific result handling
 *
 * This class extends the base SearchResult with OCR text search specific features.
 *
 * When detailed results are enabled, this API provides access to additional
 * metadata including filename, hidden status, and time information.
 */
class OcrTextResultAPI
{
public:
    /**
     * @brief Constructor
     * @param result The SearchResult object to operate on
     */
    OcrTextResultAPI(SearchResult &result);

    // ==================== OCR Content Attributes ====================

    /**
     * @brief Get the OCR extracted text content
     * @return The OCR extracted text as QString
     */
    QString ocrContent() const;

    /**
     * @brief Set the OCR extracted text content
     * @param content The OCR extracted text to set
     */
    void setOcrContent(const QString &content);

    /**
     * @brief Get the highlighted OCR content preview
     * @return The highlighted content as QString
     */
    QString highlightedContent() const;

    /**
     * @brief Set the highlighted OCR content preview
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

private:
    SearchResult &m_result;
};

DFM_SEARCH_END_NS

#endif   // OCRTEXTSEARCHAPI_H
