// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/contentretriever.h>
#include <dfm-search/dsearch_global.h>
#include <dfm-search/field_names.h>

#include "utils/contenthighlighter.h"
#include "utils/highlightoptions_p.h"
#include "utils/previewoptions_p.h"
#include "utils/previewresult_p.h"
#include "utils/searchutility.h"

#include <QDebug>
#include <QFileInfo>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

#include <lucene++/Document.h>
#include <lucene++/FSDirectory.h>
#include <lucene++/IndexReader.h>
#include <lucene++/IndexSearcher.h>
#include <lucene++/LuceneHeaders.h>
#include <lucene++/Term.h>
#include <lucene++/TermQuery.h>
#include <lucene++/TopDocs.h>
#include <optional>

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

namespace {

const wchar_t *contentFieldName(SearchType type)
{
    return (type == SearchType::Ocr)
            ? LuceneFieldNames::OcrText::kOcrContents
            : LuceneFieldNames::Content::kContents;
}

const wchar_t *pathFieldName(SearchType type)
{
    return (type == SearchType::Ocr)
            ? LuceneFieldNames::OcrText::kPath
            : LuceneFieldNames::Content::kPath;
}

QString defaultIndexDirectoryForType(SearchType type)
{
    return (type == SearchType::Ocr)
            ? Global::ocrTextIndexDirectory()
            : Global::contentIndexDirectory();
}

QString storedContentFromDocument(const DocumentPtr &doc, SearchType type)
{
    if (!doc) {
        return {};
    }

    const String contentField = doc->get(contentFieldName(type));
    if (contentField.empty()) {
        return {};
    }

    return QString::fromStdWString(contentField);
}

QStringList splitKeywords(const QString &keyword)
{
    if (keyword.isEmpty()) {
        return {};
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return keyword.split(',', Qt::SkipEmptyParts);
#else
    return keyword.split(',', QString::SkipEmptyParts);
#endif
}

DocumentPtr findDocumentByPath(const SearcherPtr &searcher,
                               const QString &path,
                               SearchType type)
{
    TermPtr term = newLucene<Term>(pathFieldName(type), path.toStdWString());
    QueryPtr query = newLucene<TermQuery>(term);

    TopDocsPtr topDocs = searcher->search(query, 1);
    if (!topDocs || topDocs->totalHits == 0) {
        return nullptr;
    }

    return searcher->doc(topDocs->scoreDocs[0]->doc);
}

struct CachedIndexContext
{
    QString indexDirectory;
    FSDirectoryPtr directory;
    IndexReaderPtr reader;
    SearcherPtr searcher;
};

/**
 * @brief Route SearchType::Semantic to Content or Ocr based on file extension.
 * @return The resolved SearchType, or std::nullopt if the extension matches neither category.
 */
static std::optional<SearchType> resolveSemanticType(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix.isEmpty())
        return std::nullopt;

    if (SearchUtility::semanticDocExtensions().contains(suffix))
        return SearchType::Content;
    if (SearchUtility::semanticPicExtensions().contains(suffix))
        return SearchType::Ocr;

    return std::nullopt;
}

}   // namespace

// ── HighlightOptions (Pimpl) ──────────────────────────────────────────

HighlightOptions::HighlightOptions()
    : d(new HighlightOptionsPrivate)
{
}

HighlightOptions::~HighlightOptions() = default;

HighlightOptions::HighlightOptions(const HighlightOptions &other) = default;

HighlightOptions &HighlightOptions::operator=(const HighlightOptions &other) = default;

int HighlightOptions::maxPreviewLength() const
{
    return d->maxPreviewLength;
}

void HighlightOptions::setMaxPreviewLength(int length)
{
    d->maxPreviewLength = length;
}

int HighlightOptions::positioningMaxLength() const
{
    return d->positioningMaxLength;
}

void HighlightOptions::setPositioningMaxLength(int length)
{
    d->positioningMaxLength = length;
}

bool HighlightOptions::enableHtml() const
{
    return d->enableHtml;
}

void HighlightOptions::setEnableHtml(bool enable)
{
    d->enableHtml = enable;
}

// ── PreviewOptions (Pimpl) ─────────────────────────────────────────────

PreviewOptions::PreviewOptions()
    : d(new PreviewOptionsPrivate)
{
}

PreviewOptions::~PreviewOptions() = default;

PreviewOptions::PreviewOptions(const PreviewOptions &other) = default;

PreviewOptions &PreviewOptions::operator=(const PreviewOptions &other) = default;

int PreviewOptions::offset() const
{
    return d->offset;
}

void PreviewOptions::setOffset(int offset)
{
    d->offset = offset;
}

int PreviewOptions::maxLength() const
{
    return d->maxLength;
}

void PreviewOptions::setMaxLength(int length)
{
    d->maxLength = length;
}

QString PreviewOptions::keyword() const
{
    return d->keyword;
}

void PreviewOptions::setKeyword(const QString &keyword)
{
    d->keyword = keyword;
}

// ── PreviewResult (Pimpl) ──────────────────────────────────────────────

PreviewResult::PreviewResult()
    : d(new PreviewResultPrivate)
{
}

PreviewResult::~PreviewResult() = default;

PreviewResult::PreviewResult(const PreviewResult &other) = default;

PreviewResult &PreviewResult::operator=(const PreviewResult &other) = default;

QString PreviewResult::content() const
{
    return d->content;
}

int PreviewResult::charCount() const
{
    return d->charCount;
}

int PreviewResult::keywordOffset() const
{
    return d->keywordOffset;
}

// ── ContentRetriever ───────────────────────────────────────────────────

struct ContentRetriever::Private
{
    QString contentIndexDirectory;
    QString ocrIndexDirectory;
    mutable QMutex mutex;
    mutable QHash<int, CachedIndexContext> cacheByType;

    CachedIndexContext *ensureIndexContext(SearchType type, const QString &indexDir) const
    {
        CachedIndexContext &ctx = cacheByType[static_cast<int>(type)];
        if (ctx.searcher && ctx.reader && ctx.directory && ctx.indexDirectory == indexDir) {
            try {
                if (!ctx.reader->isCurrent()) {
                    IndexReaderPtr reopened = ctx.reader->reopen(true);
                    if (reopened != ctx.reader) {
                        ctx.reader = reopened;
                        ctx.searcher = newLucene<IndexSearcher>(ctx.reader);
                    }
                }
                return &ctx;
            } catch (const LuceneException &e) {
                qWarning() << "ContentRetriever: failed to refresh index reader"
                           << QString::fromStdWString(e.getError());
                ctx = {};
            } catch (const std::exception &e) {
                qWarning() << "ContentRetriever: failed to refresh index reader" << e.what();
                ctx = {};
            }
        }

        try {
            ctx.indexDirectory = indexDir;
            ctx.directory = FSDirectory::open(indexDir.toStdWString());
            if (!IndexReader::indexExists(ctx.directory)) {
                ctx = {};
                return nullptr;
            }

            ctx.reader = IndexReader::open(ctx.directory, true);
            ctx.searcher = newLucene<IndexSearcher>(ctx.reader);
            return &ctx;
        } catch (const LuceneException &e) {
            qWarning() << "ContentRetriever: failed to open index"
                       << QString::fromStdWString(e.getError());
        } catch (const std::exception &e) {
            qWarning() << "ContentRetriever: failed to open index" << e.what();
        }

        ctx = {};
        return nullptr;
    }
};

ContentRetriever::ContentRetriever(QObject *parent)
    : QObject(parent),
      d(std::make_unique<Private>())
{
}

ContentRetriever::~ContentRetriever() = default;

void ContentRetriever::setIndexDirectory(SearchType type, const QString &indexDirectory)
{
    if (type != SearchType::Content && type != SearchType::Ocr) {
        return;
    }

    QMutexLocker locker(&d->mutex);
    if (type == SearchType::Ocr) {
        d->ocrIndexDirectory = indexDirectory;
    } else {
        d->contentIndexDirectory = indexDirectory;
    }
    d->cacheByType.remove(static_cast<int>(type));
}

QString ContentRetriever::indexDirectory(SearchType type) const
{
    if (type == SearchType::Ocr) {
        return d->ocrIndexDirectory.isEmpty()
                ? defaultIndexDirectoryForType(type)
                : d->ocrIndexDirectory;
    }
    if (type == SearchType::Content) {
        return d->contentIndexDirectory.isEmpty()
                ? defaultIndexDirectoryForType(type)
                : d->contentIndexDirectory;
    }

    return {};
}

QString ContentRetriever::fetchHighlight(const QString &path,
                                         const QString &keyword,
                                         SearchType type,
                                         const HighlightOptions &options) const
{
    if (path.isEmpty() || keyword.isEmpty()) return {};

    // Route SearchType::Semantic to Content or Ocr based on file extension
    if (type == SearchType::Semantic) {
        std::optional<SearchType> resolved = resolveSemanticType(path);
        if (!resolved) {
            qWarning() << "ContentRetriever: cannot route Semantic type for" << path
                       << "- extension does not match doc or pic suffixes";
            return {};
        }
        type = *resolved;
    } else if (type != SearchType::Content && type != SearchType::Ocr) {
        return {};
    }

    const QStringList keywords = splitKeywords(keyword);
    if (keywords.isEmpty()) return {};

    const QString indexDir = indexDirectory(type);

    QMutexLocker locker(&d->mutex);
    CachedIndexContext *ctx = d->ensureIndexContext(type, indexDir);
    if (!ctx || !ctx->searcher) {
        return {};
    }

    try {
        const DocumentPtr doc = findDocumentByPath(ctx->searcher, path, type);
        const QString content = storedContentFromDocument(doc, type);
        if (content.isEmpty()) {
            return {};
        }

        return ContentHighlighter::customHighlight(
                keywords, content, options.maxPreviewLength(), options.enableHtml(),
                options.positioningMaxLength());
    } catch (const LuceneException &e) {
        qWarning() << "ContentRetriever: error fetching highlight for" << path
                   << QString::fromStdWString(e.getError());
        return {};
    } catch (const std::exception &e) {
        qWarning() << "ContentRetriever: std error for" << path << e.what();
        return {};
    }
}

QMap<QString, QString> ContentRetriever::fetchHighlights(const QStringList &paths,
                                                         const QString &keyword,
                                                         SearchType type,
                                                         const HighlightOptions &options) const
{
    QMap<QString, QString> results;
    if (paths.isEmpty() || keyword.isEmpty()) return results;

    if (type != SearchType::Content && type != SearchType::Ocr && type != SearchType::Semantic)
        return results;

    const QStringList keywords = splitKeywords(keyword);
    if (keywords.isEmpty()) return results;

    QMutexLocker locker(&d->mutex);

    for (const QString &path : paths) {
        // For Semantic, route each path individually based on its extension
        SearchType effectiveType = type;
        if (type == SearchType::Semantic) {
            std::optional<SearchType> resolved = resolveSemanticType(path);
            if (!resolved) {
                qWarning() << "ContentRetriever: cannot route Semantic type for" << path
                           << "- extension does not match doc or pic suffixes";
                results.insert(path, {});
                continue;
            }
            effectiveType = *resolved;
        }

        const QString indexDir = indexDirectory(effectiveType);
        CachedIndexContext *ctx = d->ensureIndexContext(effectiveType, indexDir);
        if (!ctx || !ctx->searcher) {
            results.insert(path, {});
            continue;
        }

        try {
            const DocumentPtr doc = findDocumentByPath(ctx->searcher, path, effectiveType);
            const QString content = storedContentFromDocument(doc, effectiveType);
            if (content.isEmpty()) {
                results.insert(path, {});
                continue;
            }

            results.insert(path, ContentHighlighter::customHighlight(
                                         keywords, content, options.maxPreviewLength(), options.enableHtml(),
                                         options.positioningMaxLength()));
        } catch (const LuceneException &e) {
            qWarning() << "ContentRetriever: error for" << path
                       << QString::fromStdWString(e.getError());
            results.insert(path, {});
        } catch (const std::exception &e) {
            qWarning() << "ContentRetriever: std error for" << path << e.what();
            results.insert(path, {});
        }
    }

    return results;
}

QString ContentRetriever::fetchContent(const QString &path, SearchType type) const
{
    if (path.isEmpty()) return {};

    if (type == SearchType::Semantic) {
        std::optional<SearchType> resolved = resolveSemanticType(path);
        if (!resolved) {
            qWarning() << "ContentRetriever: cannot route Semantic type for" << path
                       << "- extension does not match doc or pic suffixes";
            return {};
        }
        type = *resolved;
    } else if (type != SearchType::Content && type != SearchType::Ocr) {
        return {};
    }

    const QString indexDir = indexDirectory(type);

    QMutexLocker locker(&d->mutex);
    CachedIndexContext *ctx = d->ensureIndexContext(type, indexDir);
    if (!ctx || !ctx->searcher) {
        return {};
    }

    try {
        return storedContentFromDocument(findDocumentByPath(ctx->searcher, path, type), type);
    } catch (const LuceneException &e) {
        qWarning() << "ContentRetriever: error fetching content for" << path
                   << QString::fromStdWString(e.getError());
        return {};
    } catch (const std::exception &e) {
        qWarning() << "ContentRetriever: std error for" << path << e.what();
        return {};
    }
}

QMap<QString, QString> ContentRetriever::fetchContents(const QStringList &paths,
                                                       SearchType type) const
{
    QMap<QString, QString> results;
    if (paths.isEmpty()) return results;

    if (type != SearchType::Content && type != SearchType::Ocr && type != SearchType::Semantic)
        return results;

    QMutexLocker locker(&d->mutex);

    for (const QString &path : paths) {
        SearchType effectiveType = type;
        if (type == SearchType::Semantic) {
            std::optional<SearchType> resolved = resolveSemanticType(path);
            if (!resolved) {
                qWarning() << "ContentRetriever: cannot route Semantic type for" << path
                           << "- extension does not match doc or pic suffixes";
                results.insert(path, {});
                continue;
            }
            effectiveType = *resolved;
        }

        const QString indexDir = indexDirectory(effectiveType);
        CachedIndexContext *ctx = d->ensureIndexContext(effectiveType, indexDir);
        if (!ctx || !ctx->searcher) {
            results.insert(path, {});
            continue;
        }

        try {
            results.insert(path, storedContentFromDocument(findDocumentByPath(ctx->searcher, path, effectiveType), effectiveType));
        } catch (const LuceneException &e) {
            qWarning() << "ContentRetriever: error for" << path
                       << QString::fromStdWString(e.getError());
            results.insert(path, {});
        } catch (const std::exception &e) {
            qWarning() << "ContentRetriever: std error for" << path << e.what();
            results.insert(path, {});
        }
    }

    return results;
}

PreviewResult ContentRetriever::fetchPreview(const QString &path, SearchType type,
                                             const PreviewOptions &options) const
{
    PreviewResult result;
    if (path.isEmpty()) return result;

    // Route SearchType::Semantic to Content or Ocr based on file extension
    if (type == SearchType::Semantic) {
        std::optional<SearchType> resolved = resolveSemanticType(path);
        if (!resolved) {
            qWarning() << "ContentRetriever: cannot route Semantic type for" << path
                       << "- extension does not match doc or pic suffixes";
            return result;
        }
        type = *resolved;
    } else if (type != SearchType::Content && type != SearchType::Ocr) {
        return result;
    }

    const QString indexDir = indexDirectory(type);

    QMutexLocker locker(&d->mutex);
    CachedIndexContext *ctx = d->ensureIndexContext(type, indexDir);
    if (!ctx || !ctx->searcher) {
        return result;
    }

    try {
        const DocumentPtr doc = findDocumentByPath(ctx->searcher, path, type);
        const QString content = storedContentFromDocument(doc, type);
        if (content.isEmpty()) {
            return result;
        }

        // Keep preview metadata in the same QString character unit as offset/keywordOffset.
        result.d->charCount = content.size();
        int keywordOffset = -1;
        const QString snippet = ContentHighlighter::previewSnippet(
                content, options.offset(), options.maxLength(),
                options.keyword(), &keywordOffset);

        result.d->content = snippet;
        result.d->keywordOffset = keywordOffset;
    } catch (const LuceneException &e) {
        qWarning() << "ContentRetriever: error fetching preview for" << path
                   << QString::fromStdWString(e.getError());
    } catch (const std::exception &e) {
        qWarning() << "ContentRetriever: std error for" << path << e.what();
    }

    return result;
}

DFM_SEARCH_END_NS
