// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OCRTEXT_INDEXED_STRATEGY_H
#define OCRTEXT_INDEXED_STRATEGY_H

#include "basestrategy.h"

#include <lucene++/LuceneHeaders.h>
#include <lucene++/QueryParser.h>
#include <lucene++/BooleanQuery.h>
#include <lucene++/QueryWrapperFilter.h>
#include <lucene++/WildcardQuery.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief OCR text index search strategy
 *
 * This strategy searches for text extracted from images using OCR technology.
 * It uses a Lucene index similar to content search, but with simplified logic
 * (no highlighting support).
 */
class OcrTextIndexedStrategy : public OcrTextBaseStrategy
{
    Q_OBJECT

public:
    explicit OcrTextIndexedStrategy(const SearchOptions &options, QObject *parent = nullptr);
    ~OcrTextIndexedStrategy() override;

    void search(const SearchQuery &query) override;
    void cancel() override;

private:
    // Initialize index directory
    void initializeIndexing();

    // Perform OCR text search
    void performOcrTextSearch(const SearchQuery &query);

    // Build Lucene query
    Lucene::QueryPtr buildLuceneQuery(const SearchQuery &query, const Lucene::AnalyzerPtr &analyzer, const QString &searchPath);

    // Helper for simple queries
    Lucene::QueryPtr buildSimpleOcrContentsQuery(
            const SearchQuery &query,
            const Lucene::QueryParserPtr &ocrContentsParser);

    // Helper for "standard" boolean logic
    Lucene::QueryPtr buildStandardBooleanOcrContentsQuery(
            const SearchQuery &query,
            const Lucene::QueryParserPtr &ocrContentsParser);

    // Helper for "advanced" mixed AND logic (searches "ocr_contents" and "filename")
    Lucene::QueryPtr buildAdvancedAndQuery(
            const SearchQuery &query,
            const Lucene::QueryParserPtr &ocrContentsParser,
            const Lucene::AnalyzerPtr &analyzer);

    // Process search results
    void processSearchResults(const Lucene::IndexSearcherPtr &searcher,
                              const Lucene::Collection<Lucene::ScoreDocPtr> &scoreDocs);

    QString m_indexDir;
    Lucene::QueryPtr m_currentQuery;
    QStringList m_keywords;
};

DFM_SEARCH_END_NS

#endif   // OCRTEXT_INDEXED_STRATEGY_H
