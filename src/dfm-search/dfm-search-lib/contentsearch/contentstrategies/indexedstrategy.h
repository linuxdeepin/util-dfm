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
    SearchResultList performContentSearch(const SearchQuery &query);

    // 构建Lucene查询
    Lucene::QueryPtr buildLuceneQuery(const SearchQuery &query, const Lucene::AnalyzerPtr &analyzer);

    // 处理搜索结果
    SearchResultList processSearchResults(const Lucene::IndexSearcherPtr &searcher,
                                          const Lucene::Collection<Lucene::ScoreDocPtr> &scoreDocs);

    QString m_indexDir;
    std::atomic<bool> m_searching { false };
};

DFM_SEARCH_END_NS

#endif   // CONTENT_INDEXED_STRATEGY_H
