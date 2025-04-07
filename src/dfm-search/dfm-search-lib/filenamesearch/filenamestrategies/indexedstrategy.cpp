#include "indexedstrategy.h"
#include "utils/searchutility.h"

#include <unistd.h>
#include <sys/types.h>

#include <QDir>
#include <QDateTime>
#include <QFileInfo>

DFM_SEARCH_BEGIN_NS

FileNameIndexedStrategy::FileNameIndexedStrategy(const SearchOptions &options, QObject *parent)
    : FileNameBaseStrategy(options, parent)
{
    // 初始化索引搜索能力
    initializeIndexing();
}

FileNameIndexedStrategy::~FileNameIndexedStrategy() = default;

void FileNameIndexedStrategy::initializeIndexing()
{
    // 获取索引目录路径
    m_indexDir = SearchUtility::getAnythingIndexDirectory();

    // 检查索引是否存在
    if (!QFileInfo::exists(m_indexDir)) {
        qWarning() << "Index directory does not exist:" << m_indexDir;
    }
}

void FileNameIndexedStrategy::search(const SearchQuery &query)
{
    m_cancelled.store(false);
    m_results.clear();

    if (!QFileInfo::exists(m_indexDir)) {
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
        emit searchFinished(m_results);
        return;
    }

    QString keyword = query.keyword();
    bool caseSensitive = m_options.caseSensitive();
    QString searchPath = m_options.searchPath();

    // 获取文件类型设置
    FileNameOptionsAPI optionsApi(const_cast<SearchOptions &>(m_options));
    QStringList fileTypes = optionsApi.fileTypes();
    bool pinyinEnabled = optionsApi.pinyinEnabled();

    // 直接将文件类型传递给搜索方法，而不是在结果中过滤
    QStringList matchedFiles = performIndexSearch(query, searchPath, fileTypes, caseSensitive, pinyinEnabled);

    // 处理搜索结果
    int maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : INT_MAX;
    int count = 0;
    int total = matchedFiles.size();

    for (const QString &path : matchedFiles) {
        if (m_cancelled.load() || count >= maxResults) {
            break;
        }

        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) {
            continue;
        }

        // 创建搜索结果
        SearchResult result(path);
        FileNameResultAPI api(result);
        api.setSize(fileInfo.size());
        api.setModifiedTime(fileInfo.lastModified().toString());
        api.setIsDirectory(fileInfo.isDir());
        api.setFileType(fileInfo.suffix().isEmpty() ? (fileInfo.isDir() ? "directory" : "unknown") : fileInfo.suffix());

        // 添加到结果集合
        m_results.append(result);

        // 实时发送结果
        emit resultFound(result);

        count++;

        // 更新进度
        if (count % 10 == 0) {
            emit progressChanged(count, total);
        }
    }

    // 完成搜索
    emit progressChanged(count, count > 0 ? count : 1);
    emit searchFinished(m_results);
}

QStringList FileNameIndexedStrategy::performIndexSearch(const SearchQuery &query,
                                                        const QString &searchPath,
                                                        const QStringList &fileTypes,
                                                        bool caseSensitive,
                                                        bool pinyinEnabled)
{
    // 实现参考LuceneSearchEngine的核心逻辑
    QStringList results;

    try {
        // 1. 确定搜索类型
        SearchType searchType = determineSearchType(query, pinyinEnabled);

        // 2. 构建查询
        IndexQuery indexQuery = buildIndexQuery(query, searchType, caseSensitive, fileTypes);

        // 3. 从索引中获取结果
        results = executeIndexQuery(indexQuery, searchPath);

        // 4. 处理结果排序
        if (!results.isEmpty()) {
            // 首先按目录和文件分组
            QStringList dirs, files;
            for (const QString &path : results) {
                QFileInfo info(path);
                if (info.isDir()) {
                    dirs.append(path);
                } else {
                    files.append(path);
                }
            }

            // 然后合并结果（目录在前，文件在后）
            results = dirs + files;
        }
    } catch (const std::exception &e) {
        qWarning() << "FileName index search exception:" << e.what();
    }

    return results;
}

FileNameIndexedStrategy::SearchType FileNameIndexedStrategy::determineSearchType(
        const SearchQuery &query, bool pinyinEnabled) const
{
    QString keyword = query.keyword();

    // 有通配符的搜索
    if (keyword.contains('*') || keyword.contains('?')) {
        return SearchType::Wildcard;
    }

    // 布尔查询
    if (query.type() == SearchQuery::Type::Boolean || keyword.contains(' ')) {
        return SearchType::Boolean;
    }

    // 启用拼音搜索
    if (pinyinEnabled) {
        return SearchType::Pinyin;
    }

    // 默认简单搜索
    return SearchType::Simple;
}

FileNameIndexedStrategy::IndexQuery FileNameIndexedStrategy::buildIndexQuery(
        const SearchQuery &query,
        SearchType searchType,
        bool caseSensitive,
        const QStringList &fileTypes)
{
    IndexQuery result;
    result.terms.clear();
    result.fileTypes.clear();

    // 设置查询类型
    result.type = searchType;

    // 是否区分大小写
    result.caseSensitive = caseSensitive;

    // 添加文件类型过滤
    if (!fileTypes.isEmpty()) {
        result.fileTypes = fileTypes;
    }

    // 根据查询类型处理关键词
    switch (searchType) {
    case SearchType::Simple: {
        // 简单查询，直接添加关键词
        result.terms.append(query.keyword());
        break;
    }
    case SearchType::Wildcard: {
        // 通配符查询，处理通配符
        result.terms.append(query.keyword());
        break;
    }
    case SearchType::Boolean: {
        // 布尔查询，添加所有关键词
        QStringList keywords = SearchUtility::extractKeywords(query);
        result.terms = keywords;

        // 设置布尔操作符
        if (query.type() == SearchQuery::Type::Boolean) {
            result.booleanOp = query.booleanOperator();
        } else {
            // 对于空格分隔的搜索，默认使用AND
            result.booleanOp = SearchQuery::BooleanOperator::AND;
        }
        break;
    }
    case SearchType::Pinyin: {
        // 拼音查询，添加原始关键词和拼音支持
        result.terms.append(query.keyword());
        result.usePinyin = true;
        break;
    }
    }

    return result;
}

QStringList FileNameIndexedStrategy::executeIndexQuery(const IndexQuery &query, const QString &searchPath)
{
    QStringList results;

    // 这里模拟索引查询的执行
    // 在实际实现中，需要与索引系统对接

    // 根据查询类型执行不同的搜索逻辑
    switch (query.type) {
    case SearchType::Simple:
        results = executeSimpleSearch(query.terms, searchPath, query.caseSensitive, query.fileTypes);
        break;
    case SearchType::Wildcard:
        results = executeWildcardSearch(query.terms, searchPath, query.caseSensitive, query.fileTypes);
        break;
    case SearchType::Boolean:
        results = executeBooleanSearch(query.terms, query.booleanOp, searchPath, query.caseSensitive, query.fileTypes);
        break;
    case SearchType::Pinyin:
        results = executePinyinSearch(query.terms, searchPath, query.caseSensitive, query.fileTypes);
        break;
    }

    return results;
}

QStringList FileNameIndexedStrategy::executeSimpleSearch(
        const QStringList &terms,
        const QString &searchPath,
        bool caseSensitive,
        const QStringList &fileTypes)
{
    QStringList results;

    // 模拟简单搜索查询
    // 注意：这只是一个模拟示例，应替换为实际索引查询

    if (terms.isEmpty()) {
        return results;
    }

    QString term = terms.first();

    // 添加模拟结果
    for (int i = 0; i < 10; i++) {
        if (m_cancelled.load()) {
            break;
        }

        // 生成模拟文件路径
        QString fileName = QString("file_%1").arg(i);
        QString suffix = i % 2 == 0 ? ".txt" : ".pdf";
        QString path = QString("%1/%2%3").arg(searchPath, fileName, suffix);

        // 检查模拟匹配
        if (fileName.contains(term, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
            // 检查文件类型
            if (fileTypes.isEmpty() || fileTypes.contains(suffix.mid(1))) {
                results.append(path);
            }
        }
    }

    return results;
}

QStringList FileNameIndexedStrategy::executeWildcardSearch(
        const QStringList &patterns,
        const QString &searchPath,
        bool caseSensitive,
        const QStringList &fileTypes)
{
    QStringList results;

    // 模拟通配符搜索
    // 注意：这只是一个模拟示例，应替换为实际索引查询

    if (patterns.isEmpty()) {
        return results;
    }

    QString pattern = patterns.first();

    // 简化处理：将通配符替换为简单的包含搜索
    QString simplifiedPattern = pattern;
    simplifiedPattern.remove('*').remove('?');

    // 复用简单搜索逻辑
    QStringList terms;
    terms.append(simplifiedPattern);
    return executeSimpleSearch(terms, searchPath, caseSensitive, fileTypes);
}

QStringList FileNameIndexedStrategy::executeBooleanSearch(
        const QStringList &terms,
        SearchQuery::BooleanOperator op,
        const QString &searchPath,
        bool caseSensitive,
        const QStringList &fileTypes)
{
    QStringList results;

    return results;
}

QStringList FileNameIndexedStrategy::executePinyinSearch(
        const QStringList &terms,
        const QString &searchPath,
        bool caseSensitive,
        const QStringList &fileTypes)
{
    // 模拟拼音搜索
    // 注意：这只是一个模拟示例，应替换为实际的拼音索引查询

    QStringList results;

    // 简化处理：暂时复用简单搜索
    // 在实际实现中，应使用拼音索引
    return executeSimpleSearch(terms, searchPath, caseSensitive, fileTypes);
}

void FileNameIndexedStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
