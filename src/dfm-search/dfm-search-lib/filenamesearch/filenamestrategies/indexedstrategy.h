#ifndef FILENAME_INDEXED_STRATEGY_H
#define FILENAME_INDEXED_STRATEGY_H

#include "basestrategy.h"

#include <memory>

#include <lucene++/LuceneHeaders.h>

#include <dfm-search/searchquery.h>

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

class QueryBuilder;
class SearchCache;
class IndexManager;
/**
 * @brief 文件名索引搜索策略
 * 使用 Lucene 实现高性能文件搜索
 */
class FileNameIndexedStrategy : public FileNameBaseStrategy
{
    Q_OBJECT

public:
    // 内部使用的搜索类型枚举
    enum class SearchType {
        Simple,   // 简单关键词搜索
        Wildcard,   // 通配符搜索
        Boolean,   // 布尔多关键词搜索
        Pinyin,   // 拼音搜索
        FileType,   // 文件类型搜索
    };

    // 索引查询结构
    struct IndexQuery
    {
        SearchType type = SearchType::Simple;
        QStringList terms;
        QStringList fileTypes;
        bool caseSensitive = false;
        bool usePinyin = false;
        SearchQuery::BooleanOperator booleanOp = SearchQuery::BooleanOperator::AND;
    };

    explicit FileNameIndexedStrategy(const SearchOptions &options, QObject *parent = nullptr);
    ~FileNameIndexedStrategy() override;

    void search(const SearchQuery &query) override;
    void cancel() override;

private:
    // 初始化索引相关设置
    void initializeIndexing();

    // 执行索引搜索
    void performIndexSearch(const SearchQuery &query, const FileNameOptionsAPI &api);

    // 确定搜索类型
    SearchType determineSearchType(const SearchQuery &query, bool pinyinEnabled, const QStringList &fileTypes) const;

    // 构建索引查询
    IndexQuery buildIndexQuery(const SearchQuery &query,
                               SearchType searchType,
                               bool caseSensitive,
                               const QStringList &fileTypes);

    // 执行索引查询并处理结果
    void executeIndexQuery(const IndexQuery &query, const QString &searchPath);

    // 构建 Lucene 查询
    QueryPtr buildLuceneQuery(const IndexQuery &query) const;

    // 处理搜索结果
    SearchResult processSearchResult(const QString &path, const QString &type, const QString &time, const QString &size);

    // 成员变量
    QString m_indexDir;   // 索引目录路径
    mutable bool m_searchCancelled;   // 搜索取消标志
    int m_count;   // 当前找到的结果数
    int m_total;   // 预计的总结果数

    // Lucene 相关组件
    std::unique_ptr<QueryBuilder> m_queryBuilder;   // 查询构建器
    std::unique_ptr<IndexManager> m_indexManager;   // 索引管理器
};

/**
 * @brief 查询构建器类
 * 负责构建各种类型的 Lucene 查询
 */
class QueryBuilder
{
public:
    QueryBuilder();

    // 构建各种类型的查询
    QueryPtr buildTypeQuery(const QStringList &types) const;
    QueryPtr buildPinyinQuery(const QStringList &pinyins) const;
    QueryPtr buildFuzzyQuery(const QString &keyword, bool caseSensitive) const;
    QueryPtr buildBooleanQuery(const QStringList &terms, bool caseSensitive, SearchQuery::BooleanOperator op) const;
    QueryPtr buildWildcardQuery(const QString &keyword, bool caseSensitive) const;
    QueryPtr buildSimpleQuery(const QString &keyword, bool caseSensitive) const;

    // 处理字符串大小写
    String processString(const QString &str, bool caseSensitive) const;
};

/**
 * @brief 索引管理器类
 * 管理 Lucene 索引目录和读取器
 */
class IndexManager
{
public:
    IndexManager();

    // 获取索引目录
    FSDirectoryPtr getIndexDirectory(const QString &indexPath) const;
    // 获取索引读取器
    IndexReaderPtr getIndexReader(FSDirectoryPtr directory) const;
    // 获取搜索器
    SearcherPtr getSearcher(IndexReaderPtr reader) const;

private:
    mutable FSDirectoryPtr m_cachedDirectory;
    mutable IndexReaderPtr m_cachedReader;
    mutable SearcherPtr m_cachedSearcher;
    mutable QString m_cachedIndexPath;
};

DFM_SEARCH_END_NS

#endif   // FILENAME_INDEXED_STRATEGY_H
