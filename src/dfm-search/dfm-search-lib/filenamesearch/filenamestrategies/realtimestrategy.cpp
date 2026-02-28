// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "realtimestrategy.h"

#include <QDirIterator>
#include <QDateTime>
#include <QFileInfo>
#include <QStack>
#include <QElapsedTimer>
#include <QDebug>
#include <QRegularExpression>

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
    const QString searchPath = m_options.searchPath();
    const QStringList excludedPaths = m_options.searchExcludedPaths();
    const bool caseSensitive = m_options.caseSensitive();
    const bool includeHidden = m_options.includeHidden();
    const int maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : INT_MAX;

    // 获取文件类型过滤
    FileNameOptionsAPI optionsApi(const_cast<SearchOptions &>(m_options));
    const QStringList fileExts = optionsApi.fileExtensions();
    const bool detailedResults = m_options.detailedResultsEnabled();
    const bool resultFoundEnabled = m_options.resultFoundEnabled();

    // 检查搜索路径
    QFileInfo pathInfo(searchPath);
    if (!pathInfo.exists() || !pathInfo.isDir()) {
        emit errorOccurred(SearchError(SearchErrorCode::PathNotFound));
        emit searchFinished(m_results);
        return;
    }

    // 性能计时
    QElapsedTimer searchTimer;
    searchTimer.start();

    // 设置基础过滤器
    QDir::Filters dirFilters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    if (includeHidden) {
        dirFilters |= QDir::Hidden;
    }

    // 使用非递归的方式进行遍历
    QStack<QString> directoryStack;
    directoryStack.push(searchPath);

    int count = 0;
    QSet<QString> visitedDirs;   // 防止符号链接循环

    while (!directoryStack.isEmpty() && count < maxResults && !m_cancelled.load()) {
        // 取出一个目录进行处理
        QString currentDir = directoryStack.pop();

        // 避免符号链接循环
        QString canonicalPath = QFileInfo(currentDir).canonicalFilePath();
        if (visitedDirs.contains(canonicalPath)) {
            continue;
        }
        visitedDirs.insert(canonicalPath);

        // 检查是否在排除路径中
        if (std::any_of(excludedPaths.cbegin(), excludedPaths.cend(),
                        [&currentDir](const QString &excludedPath) {
                            return currentDir.startsWith(excludedPath);
                        })) {
            continue;
        }

        // 获取当前目录的内容
        QDir dir(currentDir);
        QFileInfoList entries;
        try {
            entries = dir.entryInfoList(dirFilters, QDir::Name);
        } catch (const std::exception &e) {
            qWarning() << "Permission Denied: " << dir.absolutePath();
            continue;
        }

        // 如果无法读取目录，可能是因为权限问题
        if (entries.isEmpty() && !dir.exists()) {
            qWarning() << "Permission Denied: " << dir.absolutePath();
            continue;
        }

        // 处理当前目录中的每个条目
        for (const QFileInfo &info : std::as_const(entries)) {
            if (m_cancelled.load() || count >= maxResults) {
                break;
            }

            // 将子目录添加到栈中以便后续处理
            if (info.isDir()) {
                // 检查是否为符号链接，避免循环
                if (info.isSymLink()) {
                    continue;
                }
                directoryStack.push(info.filePath());
            }

            // 检查文件名是否匹配查询
            QString fileName = info.fileName();
            bool matches = false;

            // 简单查询模式
            if (query.type() == SearchQuery::Type::Simple) {
                matches = fileName.contains(query.keyword(),
                                            caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
            }
            // 通配符查询模式
            else if (query.type() == SearchQuery::Type::Wildcard) {
                matches = matchWildcard(fileName, query.keyword(), caseSensitive);
            }
            // 布尔查询模式
            else if (query.type() == SearchQuery::Type::Boolean) {
                matches = matchBoolean(fileName, query, caseSensitive, false);
            }

            // 如果文件 suffix 过滤器不为空，检查文件 suffix 是否匹配
            if (matches && !fileExts.isEmpty()) {
                const QString &suffix = info.suffix().toLower();
                if (!fileExts.contains(suffix)) {
                    matches = false;
                }
            }

            if (matches) {
                // 创建搜索结果
                SearchResult result(info.filePath());

                if (detailedResults) {
                    FileNameResultAPI api(result);
                    api.setModifiedTime(info.lastModified().toString());
                    api.setIsDirectory(info.isDir());
                    api.setFileType(info.suffix().isEmpty() ? (info.isDir() ? "directory" : "unknown") : info.suffix());
                }

                // 实时发送结果
                if (resultFoundEnabled) {
                    emit resultFound(result);
                }

                // 添加到结果集合
                m_results.append(result);
                count++;
            }
        }
    }

    qInfo() << "Real-time filename search completed in" << searchTimer.elapsed() << "ms with" << count << "results";
    emit searchFinished(m_results);
}

bool FileNameRealTimeStrategy::matchPinyin(const QString &fileName, const QString &keyword)
{
    // 实时搜索不支持拼音匹配
    Q_UNUSED(fileName);
    Q_UNUSED(keyword);
    return false;
}

bool FileNameRealTimeStrategy::matchBoolean(const QString &fileName, const SearchQuery &query,
                                            bool caseSensitive, bool pinyinEnabled)
{
    Q_UNUSED(pinyinEnabled);   // 实时搜索不支持拼音匹配

    // 获取布尔操作符类型
    auto op = query.booleanOperator();

    // 获取子查询列表
    const auto &subQueries = query.subQueries();

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
            bool subMatch = matchBoolean(fileName, subQuery, caseSensitive, false);
            if (!subMatch) {
                return false;
            }
        }
        return true;
    }

    case SearchQuery::BooleanOperator::OR: {
        // 任一子查询匹配即可
        for (const auto &subQuery : subQueries) {
            bool subMatch = matchBoolean(fileName, subQuery, caseSensitive, false);
            if (subMatch) {
                return true;
            }
        }
        return false;
    }
    }

    return false;
}

bool FileNameRealTimeStrategy::matchWildcard(const QString &fileName, const QString &pattern, bool caseSensitive)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // 利用Qt内置的通配符匹配功能
    QRegularExpression regex = QRegularExpression::fromWildcard(
            pattern,
            caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    return regex.match(fileName).hasMatch();
#else
    QRegExp regex = QRegExp(pattern, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive, QRegExp::Wildcard);
    return regex.exactMatch(fileName);
#endif
}

void FileNameRealTimeStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
