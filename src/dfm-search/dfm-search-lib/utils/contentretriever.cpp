// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/contentretriever.h>
#include <dfm-search/field_names.h>
#include <dfm-search/dsearch_global.h>
#include "utils/contenthighlighter.h"

#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QDebug>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/TermQuery.h>
#include <lucene++/Term.h>
#include <lucene++/IndexReader.h>
#include <lucene++/IndexSearcher.h>
#include <lucene++/FSDirectory.h>
#include <lucene++/Document.h>
#include <lucene++/TopDocs.h>

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

namespace {

/**
 * @brief Get the Lucene content field name for the given search type
 */
const wchar_t *contentFieldName(SearchType type)
{
    return (type == SearchType::Ocr)
            ? LuceneFieldNames::OcrText::kOcrContents
            : LuceneFieldNames::Content::kContents;
}

/**
 * @brief Get the Lucene path field name for the given search type
 */
const wchar_t *pathFieldName(SearchType type)
{
    return (type == SearchType::Ocr)
            ? LuceneFieldNames::OcrText::kPath
            : LuceneFieldNames::Content::kPath;
}

/**
 * @brief Get the index directory path for the given search type
 */
QString indexDirectoryForType(SearchType type)
{
    return (type == SearchType::Ocr)
            ? Global::ocrTextIndexDirectory()
            : Global::contentIndexDirectory();
}

/**
 * @brief Split keyword string into a QStringList suitable for ContentHighlighter
 *
 * Supports comma-separated multi-keyword input, consistent with
 * how the search pipeline handles boolean queries.
 */
QStringList splitKeywords(const QString &keyword)
{
    if (keyword.isEmpty()) return {};
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    return keyword.split(',', Qt::SkipEmptyParts);
#else
    return keyword.split(',', QString::SkipEmptyParts);
#endif
}

}   // namespace

ContentRetriever::ContentRetriever(QObject *parent)
    : QObject(parent)
{
}

ContentRetriever::~ContentRetriever() = default;

QString ContentRetriever::fetchHighlight(const QString &path,
                                         const QString &keyword,
                                         SearchType type,
                                         const HighlightOptions &options) const
{
    if (path.isEmpty() || keyword.isEmpty()) return {};
    if (type != SearchType::Content && type != SearchType::Ocr) return {};

    QStringList keywords = splitKeywords(keyword);
    if (keywords.isEmpty()) return {};

    try {
        const QString indexDir = indexDirectoryForType(type);
        IndexReaderPtr reader = IndexReader::open(FSDirectory::open(indexDir.toStdWString()));
        IndexSearcherPtr searcher = newLucene<IndexSearcher>(reader);

        // Build a term query on the path field to find the exact document
        TermPtr term = newLucene<Term>(pathFieldName(type), path.toStdWString());
        QueryPtr query = newLucene<TermQuery>(term);

        TopDocsPtr topDocs = searcher->search(query, 1);
        if (!topDocs || topDocs->totalHits == 0) {
            return {};
        }

        DocumentPtr doc = searcher->doc(topDocs->scoreDocs[0]->doc);
        String contentField = doc->get(contentFieldName(type));
        if (contentField.empty()) {
            return {};
        }

        const QString content = QString::fromStdWString(contentField);
        return ContentHighlighter::customHighlight(
                keywords, content, options.maxPreviewLength, options.enableHtml);

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

    QStringList keywords = splitKeywords(keyword);
    if (keywords.isEmpty()) return results;

    try {
        // Open index reader once for all paths
        const QString indexDir = indexDirectoryForType(type);
        IndexReaderPtr reader = IndexReader::open(FSDirectory::open(indexDir.toStdWString()));
        IndexSearcherPtr searcher = newLucene<IndexSearcher>(reader);

        for (const QString &path : paths) {
            try {
                TermPtr term = newLucene<Term>(pathFieldName(type), path.toStdWString());
                QueryPtr query = newLucene<TermQuery>(term);

                TopDocsPtr topDocs = searcher->search(query, 1);
                if (!topDocs || topDocs->totalHits == 0) {
                    results.insert(path, {});
                    continue;
                }

                DocumentPtr doc = searcher->doc(topDocs->scoreDocs[0]->doc);
                String contentField = doc->get(contentFieldName(type));
                if (contentField.empty()) {
                    results.insert(path, {});
                    continue;
                }

                const QString content = QString::fromStdWString(contentField);
                results.insert(path, ContentHighlighter::customHighlight(keywords, content, options.maxPreviewLength, options.enableHtml));

            } catch (const LuceneException &e) {
                qWarning() << "ContentRetriever: error for" << path
                           << QString::fromStdWString(e.getError());
                results.insert(path, {});
            } catch (const std::exception &e) {
                qWarning() << "ContentRetriever: std error for" << path << e.what();
                results.insert(path, {});
            }
        }
    } catch (const LuceneException &e) {
        qWarning() << "ContentRetriever: failed to open index"
                   << QString::fromStdWString(e.getError());
    }

    return results;
}

DFM_SEARCH_END_NS
