#ifndef CONTENT_INDEXED_STRATEGY_H
#define CONTENT_INDEXED_STRATEGY_H

#include "basestrategy.h"
#include <memory>

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
    SearchResultList performContentSearch(const SearchQuery &query, 
                                       const QString &searchPath, 
                                       int maxResults);
    
    // 查找匹配内容的文件
    QStringList findMatchingContentFiles(const QStringList &keywords, 
                                       const QString &searchPath,
                                       int maxResults);
    
    // 提取高亮内容
    QString extractHighlightedContent(const QString &path, 
                                    const QStringList &keywords);
    
    QString m_indexDir;
    std::atomic<bool> m_searching { false };
};

DFM_SEARCH_END_NS

#endif // CONTENT_INDEXED_STRATEGY_H 