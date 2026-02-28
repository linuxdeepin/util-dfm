// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENT_INDEXED_STRATEGY_H
#define CONTENT_INDEXED_STRATEGY_H

#include "basestrategy.h"

#include <lucene++/LuceneHeaders.h>
#include <lucene++/QueryParser.h>
#include <lucene++/BooleanQuery.h>
#include <lucene++/QueryWrapperFilter.h>
#include <lucene++/WildcardQuery.h>

// 前向声明
class ContentSearcher;

DFM_SEARCH_BEGIN_NS

/**
 * @brief 内容索引搜索策略
 */
class ContentIndexedStrategy : public ContentBaseStrategy
{
    Q_OBJECT

public:
    explicit ContentIndexedStrategy(const SearchOptions &options, QObject *parent = nullptr);
    ~ContentIndexedStrategy() override;

    void search(const SearchQuery &query) override;
    void cancel() override;

private:
    // 初始化索引
    void initializeIndexing();

    // 执行内容搜索
    void performContentSearch(const SearchQuery &query);

    // Build Lucene query
    Lucene::QueryPtr buildLuceneQuery(const SearchQuery &query, const Lucene::AnalyzerPtr &analyzer, const QString &searchPath);
    // Helper for simple queries (original logic for "contents" field)
    Lucene::QueryPtr buildSimpleContentsQuery(
            const SearchQuery &query,
            const Lucene::QueryParserPtr &contentsParser);

    // Helper for "standard" boolean logic (original logic for "contents" field, handles AND/OR)
    Lucene::QueryPtr buildStandardBooleanContentsQuery(
            const SearchQuery &query,
            const Lucene::QueryParserPtr &contentsParser);

    // Helper for "advanced" mixed AND logic (searches "contents" and "filename")
    Lucene::QueryPtr buildAdvancedAndQuery(
            const SearchQuery &query,   // Operator is implicitly AND
            const Lucene::QueryParserPtr &contentsParser,
            const Lucene::AnalyzerPtr &analyzer);   // Analyzer is needed to create filenameParser

    // Process search results
    void processSearchResults(const Lucene::IndexSearcherPtr &searcher,
                              const Lucene::Collection<Lucene::ScoreDocPtr> &scoreDocs);

    QString m_indexDir;
    Lucene::QueryPtr m_currentQuery;   // 存储当前查询
    QStringList m_keywords;
};

DFM_SEARCH_END_NS

#endif   // CONTENT_INDEXED_STRATEGY_H
