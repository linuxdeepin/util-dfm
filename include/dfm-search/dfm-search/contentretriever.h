// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENTRETRIEVER_H
#define CONTENTRETRIEVER_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QSharedDataPointer>
#include <memory>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class HighlightOptionsPrivate;
class PreviewOptionsPrivate;
class PreviewResultPrivate;

/**
 * @brief Lightweight options for highlight extraction (Pimpl-based, ABI-stable)
 *
 * New fields can be added to HighlightOptionsPrivate without breaking ABI,
 * because the public class layout is fixed at one pointer width.
 *
 * Supports implicit sharing (COW) via QSharedDataPointer.
 */
class HighlightOptions
{
public:
    HighlightOptions();
    ~HighlightOptions();
    HighlightOptions(const HighlightOptions &other);
    HighlightOptions &operator=(const HighlightOptions &other);
    HighlightOptions(HighlightOptions &&other) noexcept = default;
    HighlightOptions &operator=(HighlightOptions &&other) noexcept = default;

    int maxPreviewLength() const;
    void setMaxPreviewLength(int length);

    int positioningMaxLength() const;
    void setPositioningMaxLength(int length);

    bool enableHtml() const;
    void setEnableHtml(bool enable);

private:
    QSharedDataPointer<HighlightOptionsPrivate> d;
};

/**
 * @brief Options for precise preview extraction (Pimpl-based, ABI-stable)
 *
 * Controls offset-based content reading: either a raw slice from offset
 * (no keyword) or a keyword search starting from offset.
 *
 * Supports implicit sharing (COW) via QSharedDataPointer.
 */
class PreviewOptions
{
public:
    PreviewOptions();
    ~PreviewOptions();
    PreviewOptions(const PreviewOptions &other);
    PreviewOptions &operator=(const PreviewOptions &other);
    PreviewOptions(PreviewOptions &&other) noexcept = default;
    PreviewOptions &operator=(PreviewOptions &&other) noexcept = default;

    int offset() const;
    void setOffset(int offset);

    int maxLength() const;
    void setMaxLength(int length);

    QString keyword() const;
    void setKeyword(const QString &keyword);

private:
    QSharedDataPointer<PreviewOptionsPrivate> d;
};

/**
 * @brief Result of a preview extraction (Pimpl-based, ABI-stable)
 *
 * Contains the extracted content snippet and the position of the keyword
 * in the full document text (-1 if no keyword or not matched).
 *
 * Supports implicit sharing (COW) via QSharedDataPointer.
 */
class PreviewResult
{
public:
    PreviewResult();
    ~PreviewResult();
    PreviewResult(const PreviewResult &other);
    PreviewResult &operator=(const PreviewResult &other);
    PreviewResult(PreviewResult &&other) noexcept = default;
    PreviewResult &operator=(PreviewResult &&other) noexcept = default;

    QString content() const;
    int charCount() const;
    int keywordOffset() const;

private:
    friend class ContentRetriever;
    QSharedDataPointer<PreviewResultPrivate> d;
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
     * @brief Override the Lucene index directory for a given text search type.
     *
     * When @p indexDirectory is empty, the default global index directory for
     * the given type will be used. This is primarily useful for tests or
     * isolated business scenarios that need to point at a temporary index.
     */
    void setIndexDirectory(SearchType type, const QString &indexDirectory);

    /**
     * @brief Return the effective index directory for the given text search type.
     */
    QString indexDirectory(SearchType type) const;

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

    /**
     * @brief Synchronously fetch full stored content for a single file
     *
     * Opens the Lucene index, locates the document by path,
     * and returns the full stored content field.
     *
     * @param path Absolute file path
     * @param type SearchType::Content or SearchType::Ocr
     * @return Full content text, or empty string if not found
     */
    QString fetchContent(const QString &path, SearchType type) const;

    /**
     * @brief Synchronously fetch full stored contents for multiple files
     * @return Mapping of path -> full content (empty string if not found)
     */
    QMap<QString, QString> fetchContents(const QStringList &paths,
                                         SearchType type) const;

    /**
     * @brief Synchronously fetch a precise preview snippet for a single file
     *
     * Opens the Lucene index, locates the document by path, extracts stored
     * text, and runs ContentHighlighter::previewSnippet to produce a raw
     * content slice based on offset/maxLength/keyword.
     *
     * Unlike fetchHighlight, this does NOT simplify, add ellipsis, or
     * highlight — it returns the exact content at the requested position.
     *
     * @param path    Absolute file path
     * @param type    SearchType::Content or SearchType::Ocr
     * @param options Preview configuration (offset, maxLength, keyword)
     * @return PreviewResult with content snippet and keywordOffset
     */
    PreviewResult fetchPreview(const QString &path, SearchType type,
                               const PreviewOptions &options = {}) const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

DFM_SEARCH_END_NS

#endif   // CONTENTRETRIEVER_H
