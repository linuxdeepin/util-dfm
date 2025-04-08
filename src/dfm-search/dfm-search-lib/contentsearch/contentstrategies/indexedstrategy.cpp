#include "indexedstrategy.h"

#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTextStream>
#include <QThread>
#include <QElapsedTimer>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/QueryParser.h>
#include <lucene++/BooleanQuery.h>
#include <lucene++/QueryWrapperFilter.h>
#include <lucene++/WildcardQuery.h>

#include "ChineseAnalyzer.h"
#include "utils/searchutility.h"
#include "utils/contenthighlighter.h"

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

ContentIndexedStrategy::ContentIndexedStrategy(const SearchOptions &options, QObject *parent)
    : ContentBaseStrategy(options, parent)
{
    initializeIndexing();
}

ContentIndexedStrategy::~ContentIndexedStrategy() = default;

void ContentIndexedStrategy::initializeIndexing()
{
    // 获取索引目录
    m_indexDir = SearchUtility::contentIndexDirectory();

    // 检查索引目录是否存在
    if (!QDir(m_indexDir).exists()) {
        qWarning() << "Content index directory does not exist:" << m_indexDir;
    }
}

void ContentIndexedStrategy::search(const SearchQuery &query)
{
    m_cancelled.store(false);
    m_results.clear();
    m_searching.store(true);

    try {
        // 执行内容索引搜索
        SearchResultList contentResults = performContentSearch(query);

        // 处理和发送结果
        for (int i = 0; i < contentResults.size(); i++) {
            if (m_cancelled.load()) {
                break;
            }

            // 添加到结果集合
            m_results.append(contentResults[i]);

            // 实时发送结果
            if (Q_UNLIKELY(m_options.resultFoundEnabled()))
                emit resultFound(contentResults[i]);
        }

        // 完成搜索
        emit searchFinished(m_results);
    } catch (const std::exception &e) {
        qWarning() << "Content Index Search Exception:" << e.what();
        emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
    }

    m_searching.store(false);
}

Lucene::QueryPtr ContentIndexedStrategy::buildLuceneQuery(const SearchQuery &query, const Lucene::AnalyzerPtr &analyzer)
{
    try {
        // 创建查询解析器
        Lucene::QueryParserPtr parser = newLucene<Lucene::QueryParser>(
                Lucene::LuceneVersion::LUCENE_CURRENT,
                L"contents",
                analyzer);

        parser->setAllowLeadingWildcard(true);

        if (query.type() == SearchQuery::Type::Boolean) {
            // 处理布尔查询
            Lucene::BooleanQueryPtr booleanQuery = newLucene<Lucene::BooleanQuery>();

            // 添加所有子查询
            for (const auto &subQuery : query.subQueries()) {
                Lucene::QueryPtr termQuery = parser->parse(subQuery.keyword().toStdWString());
                booleanQuery->add(termQuery,
                                  query.booleanOperator() == SearchQuery::BooleanOperator::AND ? Lucene::BooleanClause::MUST : Lucene::BooleanClause::SHOULD);
            }

            return booleanQuery;
        } else {
            // 处理简单查询
            return parser->parse(query.keyword().toStdWString());
        }
    } catch (const Lucene::LuceneException &e) {
        qWarning() << "Error building query:" << QString::fromStdWString(e.getError());
        return nullptr;
    }
}

SearchResultList ContentIndexedStrategy::processSearchResults(const Lucene::IndexSearcherPtr &searcher,
                                                              const Lucene::Collection<Lucene::ScoreDocPtr> &scoreDocs)
{
    SearchResultList results;

    QString searchPath = m_options.searchPath();
    for (int32_t i = 0; i < scoreDocs.size(); ++i) {
        if (m_cancelled.load()) {
            break;
        }

        try {
            Lucene::ScoreDocPtr scoreDoc = scoreDocs[i];
            Lucene::DocumentPtr doc = searcher->doc(scoreDoc->doc);
            QString path = QString::fromStdWString(doc->get(L"path"));

            if (!path.startsWith(searchPath)) {
                continue;
            }
            // 创建搜索结果
            SearchResult result(path);

            // 设置内容结果
            ContentResultAPI api(result);
            
            // 使用ContentHighlighter命名空间进行高亮
            const QString &content = QString::fromStdWString(doc->get(L"contents"));
            const QString &highlightedContent = ContentHighlighter::highlight(content, m_currentQuery, 50);
            api.setHighlightedContent(highlightedContent);

            results.append(result);

        } catch (const Lucene::LuceneException &e) {
            qWarning() << "Error processing result:" << QString::fromStdWString(e.getError());
            continue;
        }
    }

    return results;
}

SearchResultList ContentIndexedStrategy::performContentSearch(const SearchQuery &query)
{
    SearchResultList results;

    try {
        // 获取索引目录
        FSDirectoryPtr directory = FSDirectory::open(m_indexDir.toStdWString());
        if (!directory) {
            qWarning() << "Failed to open index directory:" << m_indexDir;
            emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexNotFound));
            return results;
        }

        // 获取索引读取器
        IndexReaderPtr reader = IndexReader::open(directory, true);
        if (!reader || reader->numDocs() == 0) {
            qWarning() << "Index is empty or cannot be opened";
            emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexNotFound));
            return results;
        }

        // 创建搜索器
        IndexSearcherPtr searcher = newLucene<IndexSearcher>(reader);

        // 创建分析器
        AnalyzerPtr analyzer = newLucene<ChineseAnalyzer>();

        // 构建查询
        m_currentQuery = buildLuceneQuery(query, analyzer);
        if (!m_currentQuery) {
            qWarning() << "Failed to build Lucene query";
            emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
            return results;
        }

        // 执行搜索
        int32_t maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : reader->numDocs();
        TopDocsPtr topDocs = searcher->search(m_currentQuery, maxResults);
        Collection<ScoreDocPtr> scoreDocs = topDocs->scoreDocs;

        // 处理搜索结果
        results = processSearchResults(searcher, scoreDocs);

    } catch (const LuceneException &e) {
        qWarning() << "Lucene search exception:" << QString::fromStdWString(e.getError());
        emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
    } catch (const std::exception &e) {
        qWarning() << "Standard exception:" << e.what();
        emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
    }

    return results;
}

void ContentIndexedStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
