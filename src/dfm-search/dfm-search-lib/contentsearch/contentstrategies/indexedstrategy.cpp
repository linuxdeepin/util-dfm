#include "indexedstrategy.h"

#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTextStream>
#include <QThread>

DFM_SEARCH_BEGIN_NS

ContentIndexedStrategy::ContentIndexedStrategy(const SearchOptions &options, QObject *parent)
    : ContentBaseStrategy(options, parent)
{
    // 初始化内容索引
    initializeIndexing();
}

ContentIndexedStrategy::~ContentIndexedStrategy() = default;

void ContentIndexedStrategy::initializeIndexing()
{
    // 获取索引目录
    m_indexDir = QDir::homePath() + "/.config/deepin/dde-file-manager/index";

    // 检查索引是否存在
    if (!QDir(m_indexDir).exists()) {
        qWarning() << "Index dir is not exitsts:" << m_indexDir;
    }
}

void ContentIndexedStrategy::search(const SearchQuery &query)
{
    m_cancelled.store(false);
    m_results.clear();
    m_searching.store(true);

    int maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : INT_MAX;
    QString searchPath = m_options.searchPath();

    try {
        // 执行内容索引搜索
        SearchResultList contentResults = performContentSearch(query, searchPath, maxResults);

        // 处理和发送结果
        for (int i = 0; i < contentResults.size(); i++) {
            if (m_cancelled.load()) {
                break;
            }

            // 添加到结果集合
            m_results.append(contentResults[i]);

            // 实时发送结果
            emit resultFound(contentResults[i]);

            // 更新进度
            if (i % 10 == 0) {
                emit progressChanged(i, contentResults.size());
            }
        }

        // 完成搜索
        emit searchFinished(m_results);
    } catch (const std::exception &e) {
        qWarning() << "Content Index Search Exception:" << e.what();
        emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
    }

    m_searching.store(false);
}

SearchResultList ContentIndexedStrategy::performContentSearch(const SearchQuery &query,
                                                              const QString &searchPath,
                                                              int maxResults)
{
    SearchResultList results;

    // 1. 处理布尔查询
    QStringList keywords;
    if (query.type() == SearchQuery::Type::Boolean) {
        // 收集所有子查询的关键词
        for (const auto &subQuery : query.subQueries()) {
            keywords.append(subQuery.keyword());
        }
        if (keywords.isEmpty()) {
            keywords.append(query.keyword());
        }
    } else {
        // 简单查询
        keywords.append(query.keyword());
    }

    // 2. 从索引中查找匹配项
    // 此处为模拟实现，实际应使用内容索引系统
    QStringList matchedFiles = findMatchingContentFiles(keywords, searchPath, maxResults);

    // 3. 处理搜索结果，提取高亮内容
    for (const QString &path : matchedFiles) {
        if (results.size() >= maxResults || m_cancelled.load()) {
            break;
        }

        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) {
            continue;
        }

        // 创建搜索结果
        SearchResult result(path);

        // 提取内容片段并高亮
        QString highlightedContent = extractHighlightedContent(path, keywords);
        ContentResultAPI api(result);
        api.setHighlightedContent(highlightedContent);

        results.append(result);
    }

    return results;
}

QStringList ContentIndexedStrategy::findMatchingContentFiles(const QStringList &keywords,
                                                             const QString &searchPath,
                                                             int maxResults)
{
    QStringList results;

    // 模拟索引搜索结果
    // 在实际实现中，这里应该是调用内容索引系统的API
    for (int i = 0; i < 20 && results.size() < maxResults && !m_cancelled.load(); i++) {
        QString path = QString("%1/document_%2.txt").arg(searchPath).arg(i);

        // 模拟匹配检查
        bool allMatch = true;
        for (const QString &keyword : keywords) {
            // 假设这个文件包含所有关键词
            if (i % 3 != 0) {   // 只有1/3的文件会匹配
                allMatch = false;
                break;
            }
        }

        if (allMatch) {
            results.append(path);
        }
    }

    return results;
}

QString ContentIndexedStrategy::extractHighlightedContent(const QString &path,
                                                          const QStringList &keywords)
{
    // 这里应该是从索引中获取预处理的高亮内容
    // 为了简单起见，我们模拟一个高亮内容返回

    QString content = QString("This is a sample content from file %1. ").arg(path);

    // 添加关键词
    for (const QString &keyword : keywords) {
        content += QString("It contains the keyword <b style=\"color:red;\">%1</b>. ").arg(keyword);
    }

    return content;
}

void ContentIndexedStrategy::cancel()
{
    m_cancelled.store(true);

    // 等待当前搜索完成
    int timeout = 100;   // 最多等待10秒
    while (m_searching.load() && timeout > 0) {
        QThread::msleep(100);
        timeout--;
    }
}

DFM_SEARCH_END_NS
