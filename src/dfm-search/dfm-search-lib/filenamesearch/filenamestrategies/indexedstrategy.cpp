// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "indexedstrategy.h"
#include "utils/cancellablecollector.h"
#include "utils/searchutility.h"
#include "utils/lucenequeryutils.h"

#include <unistd.h>
#include <sys/types.h>

#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>

#include "3rdparty/fulltext/chineseanalyzer.h"

DFM_SEARCH_BEGIN_NS

//--------------------------------------------------------------------
// QueryBuilder 实现
//--------------------------------------------------------------------

QueryBuilder::QueryBuilder()
{
}

Lucene::QueryPtr QueryBuilder::buildTypeQuery(const QStringList &types) const
{
    if (types.isEmpty()) {
        return nullptr;
    }

    BooleanQueryPtr typeQuery = newLucene<BooleanQuery>();

    for (const QString &type : types) {
        QString cleanType = type.trimmed().toLower();
        if (!cleanType.isEmpty()) {
            QueryPtr termQuery = newLucene<TermQuery>(
                    newLucene<Term>(L"file_type",
                                    StringUtils::toUnicode(cleanType.toStdString())));
            typeQuery->add(termQuery, BooleanClause::SHOULD);
        }
    }

    return typeQuery;
}

Lucene::QueryPtr QueryBuilder::buildExtQuery(const QStringList &extensions) const
{
    if (extensions.isEmpty()) {
        return nullptr;
    }

    BooleanQueryPtr extQuery = newLucene<BooleanQuery>();

    for (const QString &ext : extensions) {
        QString cleanExt = ext.trimmed().toLower();
        if (!cleanExt.isEmpty()) {
            QueryPtr termQuery = newLucene<TermQuery>(
                    newLucene<Term>(L"file_ext",
                                    StringUtils::toUnicode(cleanExt.toStdString())));
            extQuery->add(termQuery, BooleanClause::SHOULD);
        }
    }

    return extQuery;
}

Lucene::QueryPtr QueryBuilder::buildPinyinQuery(const QStringList &pinyins, SearchQuery::BooleanOperator op) const
{
    if (pinyins.isEmpty()) {
        return nullptr;
    }

    BooleanQueryPtr pinyinQuery = newLucene<BooleanQuery>();

    for (const QString &pinyin : pinyins) {
        QString cleanPinyin = pinyin.trimmed();
        if (!cleanPinyin.isEmpty() && Global::isPinyinSequence(cleanPinyin)) {
            // 复用buildCommonQuery，指定pinyin字段，让分析器自动处理匹配
            QueryPtr termQuery = buildCommonQuery(cleanPinyin, false, newLucene<ChineseAnalyzer>(), "pinyin", false);
            if (termQuery) {
                pinyinQuery->add(termQuery, op == SearchQuery::BooleanOperator::AND ? BooleanClause::MUST : BooleanClause::SHOULD);
            }
        }
    }

    return pinyinQuery;
}

Lucene::QueryPtr QueryBuilder::buildPinyinAcronymQuery(const QStringList &acronyms, SearchQuery::BooleanOperator op) const
{
    if (acronyms.isEmpty()) {
        return nullptr;
    }

    BooleanQueryPtr acronymQuery = newLucene<BooleanQuery>();

    for (const QString &acronym : acronyms) {
        QString cleanAcronym = acronym.trimmed();
        if (!cleanAcronym.isEmpty()) {
            // 复用buildCommonQuery，指定pinyin_acronym字段，让分析器自动处理匹配
            QueryPtr termQuery = buildCommonQuery(cleanAcronym, false, newLucene<ChineseAnalyzer>(), "pinyin_acronym", false);
            if (termQuery) {
                acronymQuery->add(termQuery, op == SearchQuery::BooleanOperator::AND ? BooleanClause::MUST : BooleanClause::SHOULD);
            }
        }
    }

    return acronymQuery;
}

Lucene::QueryPtr QueryBuilder::buildCommonQuery(const QString &keyword, bool caseSensitive, const Lucene::AnalyzerPtr &analyzer, bool allowWildcard) const
{
    if (keyword.isEmpty() || !analyzer) {
        return nullptr;
    }

    Lucene::QueryParserPtr parser = newLucene<Lucene::QueryParser>(
            Lucene::LuceneVersion::LUCENE_CURRENT,
            L"file_name",
            analyzer);

    if (allowWildcard) {
        parser->setAllowLeadingWildcard(true);
    }

    return parser->parse(LuceneQueryUtils::processQueryString(keyword, caseSensitive));
}

Lucene::QueryPtr QueryBuilder::buildCommonQuery(const QString &keyword, bool caseSensitive, const Lucene::AnalyzerPtr &analyzer, const QString &fieldName, bool allowWildcard) const
{
    if (keyword.isEmpty() || !analyzer || fieldName.isEmpty()) {
        return nullptr;
    }

    Lucene::QueryParserPtr parser = newLucene<Lucene::QueryParser>(
            Lucene::LuceneVersion::LUCENE_CURRENT,
            StringUtils::toUnicode(fieldName.toStdString()),
            analyzer);

    if (allowWildcard) {
        parser->setAllowLeadingWildcard(true);
    }

    return parser->parse(LuceneQueryUtils::processQueryString(keyword, caseSensitive));
}

Lucene::QueryPtr QueryBuilder::buildSimpleQuery(const QString &keyword, bool caseSensitive, const Lucene::AnalyzerPtr &analyzer) const
{
    return buildCommonQuery(keyword, caseSensitive, analyzer, false);
}

Lucene::QueryPtr QueryBuilder::buildWildcardQuery(const QString &keyword, bool caseSensitive, const Lucene::AnalyzerPtr &analyzer) const
{
    if (keyword.isEmpty()) {
        return nullptr;
    }

    // 对于通配符查询，使用file_name_lower字段（非分词）而非file_name（分词）
    QString processedKeyword = caseSensitive ? keyword : keyword.toLower();

    // 直接构建WildcardQuery，不使用QueryParser避免分词干扰
    return newLucene<WildcardQuery>(
            newLucene<Term>(L"file_name_lower",
                            StringUtils::toUnicode(processedKeyword.toStdString())));
}

Lucene::QueryPtr QueryBuilder::buildBooleanQuery(const QStringList &terms, bool caseSensitive, SearchQuery::BooleanOperator op, const Lucene::AnalyzerPtr &analyzer) const
{
    if (terms.isEmpty() || !analyzer) {
        return nullptr;
    }

    BooleanQueryPtr booleanQuery = newLucene<BooleanQuery>();
    booleanQuery->setMaxClauseCount(1024);

    for (const QString &term : terms) {
        if (!term.isEmpty()) {
            QueryPtr termQuery = buildCommonQuery(term, caseSensitive, analyzer, false);
            if (termQuery) {
                booleanQuery->add(termQuery, op == SearchQuery::BooleanOperator::AND ? BooleanClause::MUST : BooleanClause::SHOULD);
            }
        }
    }

    return booleanQuery;
}

//--------------------------------------------------------------------
// IndexManager 实现
//--------------------------------------------------------------------

IndexManager::IndexManager()
{
}

FSDirectoryPtr IndexManager::getIndexDirectory(const QString &indexPath) const
{
    if (m_cachedDirectory && m_cachedIndexPath == indexPath) {
        return m_cachedDirectory;
    }

    m_cachedIndexPath = indexPath;
    try {
        m_cachedDirectory = FSDirectory::open(StringUtils::toUnicode(indexPath.toStdString()));
        return m_cachedDirectory;
    } catch (const LuceneException &e) {
        qWarning() << "Failed to open index directory:" << QString::fromStdWString(e.getError());
        return nullptr;
    }
}

IndexReaderPtr IndexManager::getIndexReader(FSDirectoryPtr directory) const
{
    if (!directory) {
        return nullptr;
    }

    if (m_cachedReader && m_cachedDirectory == directory) {
        return m_cachedReader;
    }

    try {
        if (IndexReader::indexExists(directory)) {
            m_cachedReader = IndexReader::open(directory, true);
            return m_cachedReader;
        }
    } catch (const LuceneException &e) {
        qWarning() << "Failed to open index reader:" << QString::fromStdWString(e.getError());
    }

    return nullptr;
}

SearcherPtr IndexManager::getSearcher(IndexReaderPtr reader) const
{
    if (!reader) {
        return nullptr;
    }

    if (m_cachedSearcher && m_cachedReader == reader) {
        return m_cachedSearcher;
    }

    try {
        m_cachedSearcher = newLucene<IndexSearcher>(reader);
        return m_cachedSearcher;
    } catch (const LuceneException &e) {
        qWarning() << "Failed to create searcher:" << QString::fromStdWString(e.getError());
        return nullptr;
    }
}

//--------------------------------------------------------------------
// FileNameIndexedStrategy 实现
//--------------------------------------------------------------------

FileNameIndexedStrategy::FileNameIndexedStrategy(const SearchOptions &options, QObject *parent)
    : FileNameBaseStrategy(options, parent)
{
    m_queryBuilder = std::make_unique<QueryBuilder>();
    m_indexManager = std::make_unique<IndexManager>();
    initializeIndexing();
}

FileNameIndexedStrategy::~FileNameIndexedStrategy() = default;

void FileNameIndexedStrategy::initializeIndexing()
{
    m_indexDir = Global::fileNameIndexDirectory();
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

    // 获取文件类型设置
    FileNameOptionsAPI optionsApi(const_cast<SearchOptions &>(m_options));

    // 执行搜索
    try {
        performIndexSearch(query, optionsApi);
    } catch (const LuceneException &e) {
        qWarning() << "Lucene search exception:" << QString::fromStdWString(e.getError());
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
    } catch (const std::exception &e) {
        qWarning() << "Standard exception:" << e.what();
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
    }

    emit searchFinished(m_results);
}

void FileNameIndexedStrategy::performIndexSearch(const SearchQuery &query, const FileNameOptionsAPI &api)
{
    bool caseSensitive = m_options.caseSensitive();
    const QString &searchPath = m_options.searchPath();
    const QStringList &searchExcludedPaths = m_options.searchExcludedPaths();

    QStringList fileTypes = api.fileTypes();
    QStringList fileExtensions = api.fileExtensions();
    bool pinyinEnabled = api.pinyinEnabled();
    bool pinyinAcronymEnabled = api.pinyinAcronymEnabled();

    // 1. 确定搜索类型
    SearchType searchType = determineSearchType(query, pinyinEnabled, pinyinAcronymEnabled, fileTypes, fileExtensions);

    // 2. 构建查询
    IndexQuery indexQuery = buildIndexQuery(query, searchType, caseSensitive, pinyinEnabled, pinyinAcronymEnabled, fileTypes, fileExtensions);

    // 3. 执行查询并处理结果
    executeIndexQuery(indexQuery, searchPath, searchExcludedPaths);
}

FileNameIndexedStrategy::SearchType FileNameIndexedStrategy::determineSearchType(
        const SearchQuery &query,
        bool pinyinEnabled,
        bool pinyinAcronymEnabled,
        const QStringList &fileTypes,
        const QStringList &fileExtensions) const
{
    QString keyword = query.keyword();
    bool hasKeyword = !keyword.isEmpty();
    bool hasFileTypes = !fileTypes.isEmpty();
    bool hasFileExts = !fileExtensions.isEmpty();
    bool isBoolean = (query.type() == SearchQuery::Type::Boolean);

    // 检查是否需要组合搜索
    bool combinedWithTypes = (hasKeyword || isBoolean) && (hasFileTypes || hasFileExts);
    if (combinedWithTypes) {
        return SearchType::Combined;
    }

    // 空关键词但有文件类型，使用文件类型搜索
    if (!hasKeyword && hasFileTypes) {
        return SearchType::FileType;
    }

    // 空关键词但有文件后缀，使用文件后缀搜索
    if (!hasKeyword && hasFileExts) {
        return SearchType::FileExt;
    }

    // 通配符查询类型（显式指定）
    if (query.type() == SearchQuery::Type::Wildcard) {
        return SearchType::Wildcard;
    }

    // 布尔查询
    if (isBoolean) {
        return SearchType::Boolean;
    }

    if ((pinyinEnabled || pinyinAcronymEnabled) && !isBoolean) {
        // 如果同时启用了拼音和拼音首字母
        if (pinyinEnabled && pinyinAcronymEnabled) {
            // 检查关键词是否为有效拼音序列
            if (hasKeyword && Global::isPinyinSequence(keyword)) {
                return SearchType::Pinyin;
            } else if (hasKeyword && Global::isPinyinAcronymSequence(keyword)) {
                // 不是有效拼音序列，但是有效的拼音首字母，fallback到拼音首字母搜索
                return SearchType::PinyinAcronym;
            } else {
                // 既不是拼音也不是有效的拼音首字母，使用简单搜索
                return SearchType::Simple;
            }
        }
        // 只启用拼音搜索
        else if (pinyinEnabled) {
            return SearchType::Pinyin;
        }
        // 只启用拼音首字母搜索
        else if (pinyinAcronymEnabled) {
            return SearchType::PinyinAcronym;
        }
    }

    // 默认简单搜索
    return SearchType::Simple;
}

FileNameIndexedStrategy::IndexQuery FileNameIndexedStrategy::buildIndexQuery(
        const SearchQuery &query,
        SearchType searchType,
        bool caseSensitive,
        bool pinyinEnabled,
        bool pinyinAcronymEnabled,
        const QStringList &fileTypes,
        const QStringList &fileExtensions)
{
    IndexQuery result;
    result.type = searchType;
    result.caseSensitive = caseSensitive;
    result.fileTypes = fileTypes;
    result.fileExtensions = fileExtensions;
    result.usePinyin = pinyinEnabled;   // 设置拼音搜索标志
    result.usePinyinAcronym = pinyinAcronymEnabled;   // 设置拼音首字母搜索标志

    switch (searchType) {
    case SearchType::Simple:
        result.terms.append(query.keyword());
        break;
    case SearchType::Wildcard:
        result.terms.append(query.keyword());
        break;
    case SearchType::Boolean:
        result.terms = SearchUtility::extractBooleanKeywords(query);
        result.booleanOp = query.type() == SearchQuery::Type::Boolean ? query.booleanOperator() : SearchQuery::BooleanOperator::AND;
        break;
    case SearchType::Pinyin:
        result.terms.append(query.keyword());
        break;
    case SearchType::PinyinAcronym:
        result.terms.append(query.keyword());
        break;
    case SearchType::FileType:
        result.fileTypes = fileTypes;
        break;
    case SearchType::FileExt:
        result.fileExtensions = fileExtensions;
        break;
    case SearchType::Combined:
        result.terms = query.type() == SearchQuery::Type::Boolean ? SearchUtility::extractBooleanKeywords(query) : QStringList { query.keyword() };
        result.booleanOp = query.type() == SearchQuery::Type::Boolean ? query.booleanOperator() : SearchQuery::BooleanOperator::AND;
        result.combineWithFileType = !fileTypes.isEmpty();
        result.combineWithFileExt = !fileExtensions.isEmpty();
        break;
    }

    return result;
}

void FileNameIndexedStrategy::executeIndexQuery(const IndexQuery &query, const QString &searchPath, const QStringList &searchExcludedPaths)
{
    // 获取索引目录
    FSDirectoryPtr directory = m_indexManager->getIndexDirectory(m_indexDir);
    if (!directory) {
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
        return;
    }

    if (!IndexReader::indexExists(directory)) {
        qWarning() << "Index does not exist:" << m_indexDir;
        emit errorOccurred(SearchError(FileNameSearchErrorCode::FileNameIndexNotFound));
        return;
    }

    // 获取索引读取器
    IndexReaderPtr reader = m_indexManager->getIndexReader(directory);
    if (!reader || reader->numDocs() == 0) {
        qWarning() << "Index is empty";
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
        return;
    }

    // 获取搜索器
    SearcherPtr searcher = m_indexManager->getSearcher(reader);
    if (!searcher) {
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
        return;
    }

    // 构建查询
    QueryPtr luceneQuery;
    try {
        luceneQuery = buildLuceneQuery(query, searchPath);
        if (!luceneQuery) {
            emit errorOccurred(SearchError(SearchErrorCode::InvalidQuery));
            return;
        }
    } catch (const LuceneException &e) {
        qWarning() << "Error building query:" << QString::fromStdWString(e.getError());
        emit errorOccurred(SearchError(SearchErrorCode::InvalidQuery));
        return;
    }
    // Measure the time taken to execute the search
    QElapsedTimer searchTimer;
    searchTimer.start();

    // 执行搜索
    int32_t maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : reader->numDocs();

    // 使用自定义 CancellableCollector 实现可中断搜索
    Collection<ScoreDocPtr> scoreDocs;
    try {
        // 创建可取消的收集器
        boost::shared_ptr<CancellableCollector> collector = newLucene<CancellableCollector>(&m_cancelled, maxResults);

        // 执行搜索，使用自定义收集器
        searcher->search(luceneQuery, collector);

        // 获取收集到的文档
        scoreDocs = collector->getScoreDocs();

        qInfo() << "Filename search execution time:" << searchTimer.elapsed() << "ms"
                << "Total hits:" << collector->getTotalHits()
                << "Collected:" << scoreDocs.size();
    } catch (const SearchCancelledException &e) {
        qInfo() << "Filename search cancelled during execution";
        return;
    }

    // Measure the time taken to process search results
    QElapsedTimer resultTimer;
    resultTimer.start();
    auto docsSize = scoreDocs.size();
    m_results.reserve(docsSize);

    // 实时处理搜索结果
    for (int i = 0; i < docsSize; i++) {
        if (m_cancelled.load()) {
            qInfo() << "Filename search cancelled";
            break;
        }

        try {
            ScoreDocPtr scoreDoc = scoreDocs[i];
            DocumentPtr doc = searcher->doc(scoreDoc->doc);
            QString path = QString::fromStdWString(doc->get(L"full_path"));

            if (!path.startsWith(searchPath)) {
                continue;
            }

            if (std::any_of(searchExcludedPaths.cbegin(), searchExcludedPaths.cend(),
                            [&path](const auto &excluded) { return path.startsWith(excluded); })) {
                continue;
            }

            if (Q_LIKELY(!m_options.includeHidden())) {
                if (QString::fromStdWString(doc->get(L"is_hidden")).toLower() == "y")
                    continue;
            }

            // 处理搜索结果
            if (Q_UNLIKELY(m_options.detailedResultsEnabled())) {
                QString type = QString::fromStdWString(doc->get(L"file_type"));
                QString time = QString::fromStdWString(doc->get(L"modify_time_str"));
                QString size = QString::fromStdWString(doc->get(L"file_size_str"));
                m_results.append(processSearchResult(path, type, time, size));
            } else {
                // perf: quickly
                SearchResult result(path);
                m_results.append(result);
            }

            // 实时发送结果
            if (Q_UNLIKELY(m_options.resultFoundEnabled()))
                emit resultFound(m_results.last());

        } catch (const LuceneException &e) {
            qWarning() << "Error processing result:" << QString::fromStdWString(e.getError());
            continue;
        }
    }

    qInfo() << "Filename result processing time:" << resultTimer.elapsed() << "ms";
}

SearchResult FileNameIndexedStrategy::processSearchResult(const QString &path, const QString &type, const QString &time, const QString &size)
{
    // 创建搜索结果
    SearchResult result(path);

    FileNameResultAPI api(result);
    api.setSize(size);
    api.setModifiedTime(time);
    api.setIsDirectory(type == "dir");
    api.setFileType(type);

    return result;
}

Lucene::QueryPtr FileNameIndexedStrategy::buildLuceneQuery(const IndexQuery &query, const QString &searchPath) const
{
    BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
    bool hasValidQuery = false;
    AnalyzerPtr analyzer = newLucene<ChineseAnalyzer>();

    switch (query.type) {
    case SearchType::Simple:
        if (!query.terms.isEmpty()) {
            QueryPtr simpleQuery = m_queryBuilder->buildSimpleQuery(query.terms.first(), query.caseSensitive, analyzer);
            if (simpleQuery) {
                finalQuery->add(simpleQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::Wildcard:
        if (!query.terms.isEmpty()) {
            QueryPtr wildcardQuery = m_queryBuilder->buildWildcardQuery(query.terms.first(), query.caseSensitive, analyzer);
            if (wildcardQuery) {
                finalQuery->add(wildcardQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::Boolean:
        if (!query.terms.isEmpty()) {
            BooleanQueryPtr booleanQuery = buildBooleanTermsQuery(query, analyzer);
            if (booleanQuery) {
                finalQuery->add(booleanQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::Pinyin:
        if (!query.terms.isEmpty()) {
            BooleanQueryPtr combinedQuery = newLucene<BooleanQuery>();

            // 添加拼音查询
            if (Global::isPinyinSequence(query.terms.first())) {
                QueryPtr pinyinQuery = m_queryBuilder->buildPinyinQuery(query.terms);
                if (pinyinQuery) {
                    combinedQuery->add(pinyinQuery, BooleanClause::SHOULD);
                    hasValidQuery = true;
                }
            }

            // 添加普通关键词查询
            QueryPtr simpleQuery = m_queryBuilder->buildSimpleQuery(query.terms.first(), query.caseSensitive, analyzer);
            if (simpleQuery) {
                combinedQuery->add(simpleQuery, BooleanClause::SHOULD);
                hasValidQuery = true;
            }

            if (hasValidQuery) {
                finalQuery->add(combinedQuery, BooleanClause::MUST);
            }
        }
        break;
    case SearchType::PinyinAcronym:
        if (!query.terms.isEmpty()) {
            BooleanQueryPtr combinedQuery = newLucene<BooleanQuery>();

            // 添加拼音首字母查询
            if (Global::isPinyinAcronymSequence(query.terms.first())) {
                QueryPtr pinyinAcronymQuery = m_queryBuilder->buildPinyinAcronymQuery(query.terms);
                if (pinyinAcronymQuery) {
                    combinedQuery->add(pinyinAcronymQuery, BooleanClause::SHOULD);
                    hasValidQuery = true;
                }
            }

            // 添加普通关键词查询
            QueryPtr simpleQuery = m_queryBuilder->buildSimpleQuery(query.terms.first(), query.caseSensitive, analyzer);
            if (simpleQuery) {
                combinedQuery->add(simpleQuery, BooleanClause::SHOULD);
                hasValidQuery = true;
            }

            if (hasValidQuery) {
                finalQuery->add(combinedQuery, BooleanClause::MUST);
            }
        }
        break;
    case SearchType::FileType:
        if (!query.fileTypes.isEmpty()) {
            QueryPtr typeQuery = m_queryBuilder->buildTypeQuery(query.fileTypes);
            if (typeQuery) {
                finalQuery->add(typeQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::FileExt:
        if (!query.fileExtensions.isEmpty()) {
            QueryPtr extQuery = m_queryBuilder->buildExtQuery(query.fileExtensions);
            if (extQuery) {
                finalQuery->add(extQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::Combined:
        if (!query.terms.isEmpty()) {
            BooleanQueryPtr combinedQuery = buildBooleanTermsQuery(query, analyzer);
            if (combinedQuery) {
                finalQuery->add(combinedQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }

        // 构建文件类型查询
        if (query.combineWithFileType && !query.fileTypes.isEmpty()) {
            QueryPtr typeQuery = m_queryBuilder->buildTypeQuery(query.fileTypes);
            if (typeQuery) {
                finalQuery->add(typeQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }

        // 构建文件后缀查询
        if (query.combineWithFileExt && !query.fileExtensions.isEmpty()) {
            QueryPtr extQuery = m_queryBuilder->buildExtQuery(query.fileExtensions);
            if (extQuery) {
                finalQuery->add(extQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    }

    // Add path prefix query optimization
    if (hasValidQuery && SearchUtility::isFilenameIndexAncestorPathsSupported()
        && SearchUtility::shouldUsePathPrefixQuery(searchPath)) {
        QueryPtr pathPrefixQuery = LuceneQueryUtils::buildPathPrefixQuery(searchPath, "ancestor_paths");
        if (pathPrefixQuery) {
            finalQuery->add(pathPrefixQuery, BooleanClause::MUST);
            qInfo() << "Using path prefix query for optimization:" << searchPath;
        }
    }

    return hasValidQuery ? finalQuery : nullptr;
}

BooleanQueryPtr FileNameIndexedStrategy::buildBooleanTermsQuery(const IndexQuery &query, const AnalyzerPtr &analyzer) const
{
    // 创建布尔查询
    BooleanQueryPtr booleanQuery = newLucene<BooleanQuery>();
    bool hasValidQuery = false;

    // 对每个搜索词创建子查询
    for (const QString &term : query.terms) {
        BooleanQueryPtr termQuery = newLucene<BooleanQuery>();
        bool termHasQuery = false;

        // 添加普通关键词查询
        QueryPtr keywordQuery = m_queryBuilder->buildSimpleQuery(term, query.caseSensitive, analyzer);
        if (keywordQuery) {
            termQuery->add(keywordQuery, BooleanClause::SHOULD);
            termHasQuery = true;
        }

        // 添加拼音查询
        if (query.usePinyin && Global::isPinyinSequence(term)) {
            QueryPtr pinyinQuery = m_queryBuilder->buildPinyinQuery(QStringList { term });
            if (pinyinQuery) {
                termQuery->add(pinyinQuery, BooleanClause::SHOULD);
                termHasQuery = true;
            }
        }

        // 添加拼音首字母查询
        if (query.usePinyinAcronym && Global::isPinyinAcronymSequence(term)) {
            QueryPtr pinyinAcronymQuery = m_queryBuilder->buildPinyinAcronymQuery(QStringList { term });
            if (pinyinAcronymQuery) {
                termQuery->add(pinyinAcronymQuery, BooleanClause::SHOULD);
                termHasQuery = true;
            }
        }

        // 将当前词的查询添加到最终查询中，维持原始bool逻辑
        if (termHasQuery) {
            booleanQuery->add(termQuery, query.booleanOp == SearchQuery::BooleanOperator::AND ? BooleanClause::MUST : BooleanClause::SHOULD);
            hasValidQuery = true;
        }
    }

    return hasValidQuery ? booleanQuery : nullptr;
}

void FileNameIndexedStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
