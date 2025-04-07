#include "realtimestrategy.h"
#include <QDirIterator>
#include <QDateTime>
#include <QFileInfo>

DFM_SEARCH_BEGIN_NS

FileNameRealTimeStrategy::FileNameRealTimeStrategy(const SearchOptions &options, QObject *parent)
    : FileNameBaseStrategy(options, parent)
{
}

FileNameRealTimeStrategy::~FileNameRealTimeStrategy() = default;

void FileNameRealTimeStrategy::search(const SearchQuery &query)
{
    m_cancelled.store(false);
    m_results.clear();

    // 从搜索选项获取参数
    QString searchPath = m_options.searchPath();
    bool caseSensitive = m_options.caseSensitive();
    bool includeHidden = m_options.includeHidden();
    QStringList excludePaths = m_options.excludePaths();
    int maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : INT_MAX;

    // 获取文件类型过滤
    FileNameOptionsAPI optionsApi(const_cast<SearchOptions &>(m_options));
    QStringList fileTypes = optionsApi.fileTypes();
    bool pinyinEnabled = optionsApi.pinyinEnabled();

    // 检查搜索路径
    QFileInfo pathInfo(searchPath);
    if (!pathInfo.exists() || !pathInfo.isDir()) {
        emit errorOccurred(SearchError(SearchErrorCode::PathNotFound));
        emit searchFinished(m_results);
        return;
    }

    // 设置遍历参数
    QDirIterator::IteratorFlags flags = QDirIterator::Subdirectories;
    QDir::Filters dirFilters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    if (includeHidden) {
        dirFilters |= QDir::Hidden;
    }

    QDirIterator it(searchPath, dirFilters, flags);
    int count = 0;
    int progress = 0;
    int totalEstimate = 1000;   // 初始估计值

    while (it.hasNext() && count < maxResults && !m_cancelled.load()) {
        QString path = it.next();
        QFileInfo info(path);

        // 检查排除路径
        bool excluded = false;
        for (const QString &excludePath : excludePaths) {
            if (path.startsWith(excludePath)) {
                excluded = true;
                break;
            }
        }

        if (excluded) {
            continue;
        }

        // 检查文件名是否匹配查询
        QString fileName = info.fileName();
        bool matches = false;

        // 简单查询模式
        if (query.type() == SearchQuery::Type::Simple) {
            matches = fileName.contains(query.keyword(),
                                        caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

            // 如果启用拼音搜索且未匹配，尝试拼音匹配
            if (pinyinEnabled && !matches) {
                matches = matchPinyin(fileName, query.keyword());
            }
        }
        // 布尔查询模式
        else if (query.type() == SearchQuery::Type::Boolean) {
            matches = matchBoolean(fileName, query, caseSensitive, pinyinEnabled);
        }

        // 如果文件类型过滤器不为空，检查文件类型是否匹配
        if (matches && !fileTypes.isEmpty()) {
            QString suffix = info.suffix().toLower();
            if (!fileTypes.contains(suffix)) {
                matches = false;
            }
        }

        if (matches) {
            // 创建搜索结果
            SearchResult result(path);
            FileNameResultAPI api(result);
            api.setSize(info.size());
            api.setModifiedTime(info.lastModified().toString());
            api.setIsDirectory(info.isDir());
            api.setFileType(info.suffix().isEmpty() ? (info.isDir() ? "directory" : "unknown") : info.suffix());

            // 添加到结果集合
            m_results.append(result);

            // 实时发送结果
            emit resultFound(result);

            count++;
        }

        // 每处理100个文件更新一次进度
        progress++;
        if (progress % 100 == 0) {
            // 动态调整总估计值
            if (progress > totalEstimate / 2) {
                totalEstimate = progress * 2;
            }
            emit progressChanged(progress, totalEstimate);
        }
    }

    // 发送最终进度
    emit progressChanged(progress, progress);

    // 搜索完成
    emit searchFinished(m_results);
}

bool FileNameRealTimeStrategy::matchPinyin(const QString &fileName, const QString &keyword)
{
    // 模拟拼音匹配逻辑
    // 实际项目中可以使用拼音匹配库
    // 这里仅返回假结果作为示例
    return false;
}

bool FileNameRealTimeStrategy::matchBoolean(const QString &fileName, const SearchQuery &query,
                                            bool caseSensitive, bool pinyinEnabled)
{
    // 获取布尔操作符类型
    auto op = query.booleanOperator();

    // 获取子查询列表
    auto subQueries = query.subQueries();

    // 如果没有子查询，使用简单匹配
    if (subQueries.isEmpty()) {
        return fileName.contains(query.keyword(),
                                 caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }

    // 根据布尔操作符处理
    switch (op) {
    case SearchQuery::BooleanOperator::AND: {
        // 所有子查询都必须匹配
        for (const auto &subQuery : subQueries) {
            bool subMatch = matchBoolean(fileName, subQuery, caseSensitive, pinyinEnabled);
            if (!subMatch) {
                return false;
            }
        }
        return true;
    }

    case SearchQuery::BooleanOperator::OR: {
        // 任一子查询匹配即可
        for (const auto &subQuery : subQueries) {
            bool subMatch = matchBoolean(fileName, subQuery, caseSensitive, pinyinEnabled);
            if (subMatch) {
                return true;
            }
        }
        return false;
    }

    case SearchQuery::BooleanOperator::NOT: {
        // 主查询匹配且子查询不匹配
        bool mainMatch = fileName.contains(query.keyword(),
                                           caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        if (!mainMatch) {
            return false;
        }

        for (const auto &subQuery : subQueries) {
            bool subMatch = matchBoolean(fileName, subQuery, caseSensitive, pinyinEnabled);
            if (subMatch) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

void FileNameRealTimeStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
