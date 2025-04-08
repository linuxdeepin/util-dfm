#include "indexedstrategy.h"
#include "utils/searchutility.h"

#include <unistd.h>
#include <sys/types.h>

#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>

DFM_SEARCH_BEGIN_NS

//--------------------------------------------------------------------
// QueryBuilder 实现
//--------------------------------------------------------------------

QueryBuilder::QueryBuilder()
{
}

Lucene::String QueryBuilder::processString(const QString &str, bool caseSensitive) const
{
    String unicodeStr = StringUtils::toUnicode(str.toStdString());
    if (caseSensitive) {
        return unicodeStr;
    } else {
        StringUtils::toLower(unicodeStr);
        return unicodeStr;
    }
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

Lucene::QueryPtr QueryBuilder::buildPinyinQuery(const QStringList &pinyins) const
{
    if (pinyins.isEmpty()) {
        return nullptr;
    }

    BooleanQueryPtr pinyinQuery = newLucene<BooleanQuery>();

    for (const QString &pinyin : pinyins) {
        QString cleanPinyin = pinyin.trimmed().toLower();
        if (!cleanPinyin.isEmpty()) {
            QueryPtr termQuery = newLucene<WildcardQuery>(
                    newLucene<Term>(L"pinyin",
                                    StringUtils::toUnicode(QString("*%1*").arg(cleanPinyin).toStdString())));
            pinyinQuery->add(termQuery, BooleanClause::SHOULD);
        }
    }

    return pinyinQuery;
}

Lucene::QueryPtr QueryBuilder::buildFuzzyQuery(const QString &keyword, bool caseSensitive) const
{
    if (keyword.isEmpty()) {
        return nullptr;
    }

    BooleanQueryPtr booleanQuery = newLucene<BooleanQuery>();
    booleanQuery->setMaxClauseCount(1024);

    QStringList terms = keyword.split(' ', Qt::SkipEmptyParts);
    for (const QString &term : terms) {
        if (term.isEmpty()) continue;

        BooleanQueryPtr termQuery = newLucene<BooleanQuery>();
        String exactTerm = processString(term, caseSensitive);

        // 精确匹配
        QueryPtr exactQuery = newLucene<TermQuery>(newLucene<Term>(L"file_name", exactTerm));
        exactQuery->setBoost(3.0);
        termQuery->add(exactQuery, BooleanClause::SHOULD);

        // 前缀匹配
        if (term.length() > 2) {
            QueryPtr prefixQuery = newLucene<PrefixQuery>(newLucene<Term>(L"file_name", exactTerm));
            prefixQuery->setBoost(2.0);
            termQuery->add(prefixQuery, BooleanClause::SHOULD);
        }

        // 通配符匹配
        if (term.length() > 3) {
            for (int i = 1; i < term.length() - 1; i++) {
                QString fuzzyPattern = term.left(i) + "*" + term.mid(i + 1);
                String wildcardTerm = processString(fuzzyPattern, caseSensitive);
                QueryPtr wildcardQuery = newLucene<WildcardQuery>(newLucene<Term>(L"file_name", wildcardTerm));
                wildcardQuery->setBoost(1.0);
                termQuery->add(wildcardQuery, BooleanClause::SHOULD);
            }
        }

        // 包含匹配
        String containsTerm = L"*" + exactTerm + L"*";
        QueryPtr containsQuery = newLucene<WildcardQuery>(newLucene<Term>(L"file_name", containsTerm));
        containsQuery->setBoost(1.5);
        termQuery->add(containsQuery, BooleanClause::SHOULD);

        booleanQuery->add(termQuery, BooleanClause::MUST);
    }

    return booleanQuery;
}

Lucene::QueryPtr QueryBuilder::buildBooleanQuery(const QStringList &terms, bool caseSensitive, SearchQuery::BooleanOperator op) const
{
    if (terms.isEmpty()) {
        return nullptr;
    }

    BooleanQueryPtr booleanQuery = newLucene<BooleanQuery>();
    booleanQuery->setMaxClauseCount(1024);

    for (const QString &term : terms) {
        if (!term.isEmpty()) {
            String termStr = L"*" + processString(term, caseSensitive) + L"*";
            TermPtr termObj = newLucene<Term>(L"file_name", termStr);
            QueryPtr termQuery = newLucene<WildcardQuery>(termObj);
            booleanQuery->add(termQuery, op == SearchQuery::BooleanOperator::AND ? BooleanClause::MUST : BooleanClause::SHOULD);
        }
    }

    return booleanQuery;
}

Lucene::QueryPtr QueryBuilder::buildWildcardQuery(const QString &keyword, bool caseSensitive) const
{
    if (keyword.isEmpty()) {
        return nullptr;
    }

    return newLucene<WildcardQuery>(
            newLucene<Term>(L"file_name",
                            processString(keyword, caseSensitive)));
}

Lucene::QueryPtr QueryBuilder::buildSimpleQuery(const QString &keyword, bool caseSensitive) const
{
    if (keyword.isEmpty()) {
        return nullptr;
    }

    String queryString = L"*" + processString(keyword, caseSensitive) + L"*";
    return newLucene<WildcardQuery>(newLucene<Term>(L"file_name", queryString));
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
    : FileNameBaseStrategy(options, parent), m_searchCancelled(false), m_count(0), m_total(0)
{
    m_queryBuilder = std::make_unique<QueryBuilder>();
    m_indexManager = std::make_unique<IndexManager>();
    initializeIndexing();
}

FileNameIndexedStrategy::~FileNameIndexedStrategy() = default;

void FileNameIndexedStrategy::initializeIndexing()
{
    m_indexDir = SearchUtility::anythingIndexDirectory();
    if (!QFileInfo::exists(m_indexDir)) {
        qWarning() << "Index directory does not exist:" << m_indexDir;
    }
}

void FileNameIndexedStrategy::search(const SearchQuery &query)
{
    m_searchCancelled = false;
    m_results.clear();
    m_count = 0;
    m_total = 0;

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
    QString searchPath = m_options.searchPath();

    QStringList fileTypes = api.fileTypes();
    bool pinyinEnabled = api.pinyinEnabled();

    // 1. 确定搜索类型
    SearchType searchType = determineSearchType(query, pinyinEnabled, fileTypes);

    // 2. 构建查询
    IndexQuery indexQuery = buildIndexQuery(query, searchType, caseSensitive, fileTypes);

    // 3. 执行查询并处理结果
    executeIndexQuery(indexQuery, searchPath);
}

FileNameIndexedStrategy::SearchType FileNameIndexedStrategy::determineSearchType(
        const SearchQuery &query, bool pinyinEnabled, const QStringList &fileTypes) const
{
    QString keyword = query.keyword();

    // 空关键词但有文件类型，使用文件类型搜索
    if (keyword.isEmpty() && !fileTypes.isEmpty()) {
        return SearchType::FileType;
    }

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
    result.type = searchType;
    result.caseSensitive = caseSensitive;
    result.fileTypes = fileTypes;

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
        result.usePinyin = true;
        break;
    case SearchType::FileType:
        result.fileTypes = fileTypes;
        break;
    }

    return result;
}

void FileNameIndexedStrategy::executeIndexQuery(const IndexQuery &query, const QString &searchPath)
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
        luceneQuery = buildLuceneQuery(query);
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
    TopDocsPtr searchResults;
    try {
        int32_t maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : reader->numDocs();
        searchResults = searcher->search(luceneQuery, maxResults);
        m_total = searchResults->totalHits;
    } catch (const LuceneException &e) {
        qWarning() << "Search execution error:" << QString::fromStdWString(e.getError());
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
        return;
    }

    qInfo() << "Search execution time:" << searchTimer.elapsed() << "ms";

    // Measure the time taken to process search results
    QElapsedTimer resultTimer;
    resultTimer.start();
    auto docsSize = searchResults->scoreDocs.size();
    m_results.reserve(docsSize);
    // 实时处理搜索结果
    for (int i = 0; i < docsSize; i++) {
        if (m_searchCancelled) {
            break;
        }

        try {
            ScoreDocPtr scoreDoc = searchResults->scoreDocs[i];
            DocumentPtr doc = searcher->doc(scoreDoc->doc);
            QString path = QString::fromStdWString(doc->get(L"full_path"));

            if (!path.startsWith(searchPath)) {
                continue;
            }

            QString type = QString::fromStdWString(doc->get(L"file_type"));
            QString time = QString::fromStdWString(doc->get(L"modify_time_str"));
            QString size = QString::fromStdWString(doc->get(L"file_size_str"));

            // 处理搜索结果
            processSearchResult(path, type, time, size);

        } catch (const LuceneException &e) {
            qWarning() << "Error processing result:" << QString::fromStdWString(e.getError());
            continue;
        }
    }

    qInfo() << "Result processing time:" << resultTimer.elapsed() << "ms";
}

void FileNameIndexedStrategy::processSearchResult(const QString &path, const QString &type, const QString &time, const QString &size)
{
    // 创建搜索结果
    SearchResult result(path);
    FileNameResultAPI api(result);
    api.setSize(size);
    api.setModifiedTime(time);
    api.setIsDirectory(type == "dir");
    api.setFileType(type);

    // 添加结果并通知
    m_results.append(result);
    emit resultFound(result);
    m_count++;
}

Lucene::QueryPtr FileNameIndexedStrategy::buildLuceneQuery(const IndexQuery &query) const
{
    BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
    bool hasValidQuery = false;

    // 根据搜索类型添加相应的查询
    switch (query.type) {
    case SearchType::Simple:
        if (!query.terms.isEmpty()) {
            QueryPtr simpleQuery = m_queryBuilder->buildSimpleQuery(query.terms.first(), query.caseSensitive);
            if (simpleQuery) {
                finalQuery->add(simpleQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::Wildcard:
        if (!query.terms.isEmpty()) {
            QueryPtr wildcardQuery = m_queryBuilder->buildWildcardQuery(query.terms.first(), query.caseSensitive);
            if (wildcardQuery) {
                finalQuery->add(wildcardQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::Boolean:
        if (!query.terms.isEmpty()) {
            QueryPtr booleanQuery = m_queryBuilder->buildBooleanQuery(query.terms, query.caseSensitive, query.booleanOp);
            if (booleanQuery) {
                finalQuery->add(booleanQuery, BooleanClause::MUST);
                hasValidQuery = true;
            }
        }
        break;
    case SearchType::Pinyin:
        if (!query.terms.isEmpty()) {
            QueryPtr pinyinQuery = m_queryBuilder->buildPinyinQuery(query.terms);
            if (pinyinQuery) {
                finalQuery->add(pinyinQuery, BooleanClause::MUST);
                hasValidQuery = true;
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
    }

    return hasValidQuery ? finalQuery : nullptr;
}

void FileNameIndexedStrategy::cancel()
{
    m_searchCancelled = true;
}

DFM_SEARCH_END_NS
