// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/contentretriever.h>
#include <dfm-search/dsearch_global.h>
#include <dfm-search/field_names.h>

#include "utils/contenthighlighter.h"

#include <QDebug>
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

}   // namespace

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
    if (type != SearchType::Content && type != SearchType::Ocr) return {};

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
                keywords, content, options.maxPreviewLength, options.enableHtml,
                options.positioningMaxLength);
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
    if (type != SearchType::Content && type != SearchType::Ocr) return results;

    const QStringList keywords = splitKeywords(keyword);
    if (keywords.isEmpty()) return results;

    const QString indexDir = indexDirectory(type);

    QMutexLocker locker(&d->mutex);
    CachedIndexContext *ctx = d->ensureIndexContext(type, indexDir);
    if (!ctx || !ctx->searcher) {
        return results;
    }

    for (const QString &path : paths) {
        try {
            const DocumentPtr doc = findDocumentByPath(ctx->searcher, path, type);
            const QString content = storedContentFromDocument(doc, type);
            if (content.isEmpty()) {
                results.insert(path, {});
                continue;
            }

            results.insert(path, ContentHighlighter::customHighlight(
                                         keywords, content, options.maxPreviewLength, options.enableHtml,
                                         options.positioningMaxLength));
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
    if (type != SearchType::Content && type != SearchType::Ocr) return {};

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
    if (type != SearchType::Content && type != SearchType::Ocr) return results;

    const QString indexDir = indexDirectory(type);

    QMutexLocker locker(&d->mutex);
    CachedIndexContext *ctx = d->ensureIndexContext(type, indexDir);
    if (!ctx || !ctx->searcher) {
        return results;
    }

    for (const QString &path : paths) {
        try {
            results.insert(path, storedContentFromDocument(findDocumentByPath(ctx->searcher, path, type), type));
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

DFM_SEARCH_END_NS
