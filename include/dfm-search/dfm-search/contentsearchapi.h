// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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
     * @brief Set file type filters for content search
     * @param extensions List of file extensions to include in search
     */
    void setFileTypeFilters(const QStringList &extensions);

    /**
     * @brief Get the current file type filters
     * @return List of file extensions
     */
    QStringList fileTypeFilters() const;

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

    // TODO (search): html

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
