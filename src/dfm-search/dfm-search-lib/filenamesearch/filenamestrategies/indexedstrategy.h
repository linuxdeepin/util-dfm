#ifndef FILENAME_INDEXED_STRATEGY_H
#define FILENAME_INDEXED_STRATEGY_H

#include "basestrategy.h"
#include <dfm-search/searchquery.h>
#include <memory>

// 前向声明
class LuceneSearchEngine;

DFM_SEARCH_BEGIN_NS

/**
 * @brief 文件名索引搜索策略
 */
class FileNameIndexedStrategy : public FileNameBaseStrategy
{
    Q_OBJECT

public:
    // 内部使用的搜索类型枚举
    enum class SearchType {
        Simple,   // 简单关键词搜索
        Wildcard,   // 通配符搜索
        Boolean,   // bool多关键词搜索
        Pinyin   // 拼音搜索
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
    QStringList performIndexSearch(const SearchQuery &query,
                                   const QString &searchPath,
                                   const QStringList &fileTypes,
                                   bool caseSensitive,
                                   bool pinyinEnabled);

    // 确定搜索类型
    SearchType determineSearchType(const SearchQuery &query, bool pinyinEnabled) const;

    // 构建索引查询
    IndexQuery buildIndexQuery(const SearchQuery &query,
                               SearchType searchType,
                               bool caseSensitive,
                               const QStringList &fileTypes);

    // 执行索引查询
    QStringList executeIndexQuery(const IndexQuery &query, const QString &searchPath);

    // 各种搜索类型的具体实现
    QStringList executeSimpleSearch(const QStringList &terms,
                                    const QString &searchPath,
                                    bool caseSensitive,
                                    const QStringList &fileTypes);

    QStringList executeWildcardSearch(const QStringList &patterns,
                                      const QString &searchPath,
                                      bool caseSensitive,
                                      const QStringList &fileTypes);

    QStringList executeBooleanSearch(const QStringList &terms,
                                     SearchQuery::BooleanOperator op,
                                     const QString &searchPath,
                                     bool caseSensitive,
                                     const QStringList &fileTypes);

    QStringList executePinyinSearch(const QStringList &terms,
                                    const QString &searchPath,
                                    bool caseSensitive,
                                    const QStringList &fileTypes);

    QString m_indexDir;   // 索引目录路径
};

DFM_SEARCH_END_NS

#endif   // FILENAME_INDEXED_STRATEGY_H
