// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENTRETRIEVER_H
#define CONTENTRETRIEVER_H

#include <QObject>
#include <QStringList>
#include <QMap>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief Lightweight options for highlight extraction
 */
struct HighlightOptions
{
    int maxPreviewLength = 200;   ///< Maximum snippet length in characters
    bool enableHtml = false;   ///< Wrap matched keywords with <b> tags
};

/**
 * @brief Retrieves highlighted content from Lucene index on demand
 *
 * Provides a standalone mechanism to fetch highlighted content snippets
 * for specific file paths without running a full search pipeline.
 *
 * Typical usage pattern:
 * 1. Perform a search with isFullTextRetrievalEnabled() = false (fast)
 * 2. Display path-only results to the user immediately
 * 3. On demand (e.g., scroll into view), call fetchHighlight() per path
 *
 * This decouples highlight extraction from the search pipeline,
 * enabling lazy-loading similar to thumbnail fetching.
 */
class ContentRetriever : public QObject
{
    Q_OBJECT

public:
    explicit ContentRetriever(QObject *parent = nullptr);
    ~ContentRetriever() override;

    /**
     * @brief Synchronously fetch highlighted content for a single file
     *
     * Opens the Lucene index, locates the document by path,
     * extracts stored text, and runs ContentHighlighter to produce
     * a highlighted snippet.
     *
     * @param path    Absolute file path
     * @param keyword Search keyword (supports comma-separated for multi-keyword)
     * @param type    SearchType::Content or SearchType::Ocr
     * @param options Highlight configuration (preview length, HTML toggle)
     * @return Highlighted snippet, or empty string if not found
     */
    QString fetchHighlight(const QString &path,
                           const QString &keyword,
                           SearchType type,
                           const HighlightOptions &options = {}) const;

    /**
     * @brief Synchronously fetch highlights for multiple files
     * @return Mapping of path -> highlighted content (empty string if not found)
     */
    QMap<QString, QString> fetchHighlights(const QStringList &paths,
                                           const QString &keyword,
                                           SearchType type,
                                           const HighlightOptions &options = {}) const;
};

DFM_SEARCH_END_NS

#endif   // CONTENTRETRIEVER_H
