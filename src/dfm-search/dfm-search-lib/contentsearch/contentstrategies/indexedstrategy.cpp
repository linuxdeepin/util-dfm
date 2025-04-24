// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
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

#include "3rdparty/fulltext/chineseanalyzer.h"
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
    m_indexDir = Global::contentIndexDirectory();

    // 检查索引目录是否存在
    if (!QDir(m_indexDir).exists()) {
        qWarning() << "Content index directory does not exist:" << m_indexDir;
    }
}

void ContentIndexedStrategy::search(const SearchQuery &query)
{
    m_cancelled.store(false);
    m_results.clear();

    try {
        // 执行内容索引搜索
        performContentSearch(query);
    } catch (const std::exception &e) {
        qWarning() << "Content Index Search Exception:" << e.what();
        emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
    }
}

Lucene::QueryPtr ContentIndexedStrategy::buildLuceneQuery(const SearchQuery &query, const Lucene::AnalyzerPtr &analyzer)
{
    try {
        // 创建查询解析器
        Lucene::QueryParserPtr parser = newLucene<Lucene::QueryParser>(
                Lucene::LuceneVersion::LUCENE_CURRENT,
                L"contents",
                analyzer);

        m_keywords.clear();
        if (query.type() == SearchQuery::Type::Boolean) {
            // 处理布尔查询
            Lucene::BooleanQueryPtr booleanQuery = newLucene<Lucene::BooleanQuery>();

            // 添加所有子查询
            for (const auto &subQuery : query.subQueries()) {
                m_keywords.append(subQuery.keyword());
                Lucene::QueryPtr termQuery = parser->parse(subQuery.keyword().toStdWString());
                booleanQuery->add(termQuery,
                                  query.booleanOperator() == SearchQuery::BooleanOperator::AND ? Lucene::BooleanClause::MUST : Lucene::BooleanClause::SHOULD);
            }

            return booleanQuery;
        } else {
            // 处理简单查询
            m_keywords.append(query.keyword());
            return parser->parse(query.keyword().toStdWString());
        }
    } catch (const Lucene::LuceneException &e) {
        qWarning() << "Error building query:" << QString::fromStdWString(e.getError());
        return nullptr;
    }
}

void ContentIndexedStrategy::processSearchResults(const Lucene::IndexSearcherPtr &searcher,
                                                  const Lucene::Collection<Lucene::ScoreDocPtr> &scoreDocs)
{
    // Measure the time taken to process search results
    QElapsedTimer resultTimer;
    resultTimer.start();

    QString searchPath = m_options.searchPath();
    const QStringList &searchExcludedPaths = m_options.searchExcludedPaths();
    auto docsSize = scoreDocs.size();

    ContentOptionsAPI optAPI(m_options);
    bool enableHTML = optAPI.isSearchResultHighlightEnabled();
    int previewLen = optAPI.maxPreviewLength() > 0 ? optAPI.maxPreviewLength() : 50;
    bool enableRetrieval = optAPI.isFullTextRetrievalEnabled();

    for (int32_t i = 0; i < docsSize; ++i) {
        if (m_cancelled.load()) {
            qInfo() << "Content search cancelled";
            break;
        }

        try {
            Lucene::ScoreDocPtr scoreDoc = scoreDocs[i];
            Lucene::DocumentPtr doc = searcher->doc(scoreDoc->doc);
            QString path = QString::fromStdWString(doc->get(L"path"));

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

            // 创建搜索结果
            SearchResult result(path);

            // 设置内容结果
            ContentResultAPI resultApi(result);

            // 使用ContentHighlighter命名空间进行高亮
            if (enableRetrieval) {
                const QString &content = QString::fromStdWString(doc->get(L"contents"));
                const QString &highlightedContent = ContentHighlighter::customHighlight(m_keywords, content, previewLen, enableHTML);
                resultApi.setHighlightedContent(highlightedContent);
            }
            // 添加到结果集合
            m_results.append(result);

            // 实时发送结果
            if (Q_UNLIKELY(m_options.resultFoundEnabled()))
                emit resultFound(result);

        } catch (const Lucene::LuceneException &e) {
            qWarning() << "Error processing result:" << QString::fromStdWString(e.getError());
            continue;
        }
    }

    qInfo() << "Content result processing time:" << resultTimer.elapsed() << "ms";
    emit searchFinished(m_results);
}

void ContentIndexedStrategy::performContentSearch(const SearchQuery &query)
{
    try {
        // 获取索引目录
        FSDirectoryPtr directory = FSDirectory::open(m_indexDir.toStdWString());
        if (!directory) {
            qWarning() << "Failed to open index directory:" << m_indexDir;
            emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexNotFound));
            return;
        }

        // 获取索引读取器
        IndexReaderPtr reader = IndexReader::open(directory, true);
        if (!reader || reader->numDocs() == 0) {
            qWarning() << "Index is empty or cannot be opened";
            emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexNotFound));
            return;
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
            return;
        }

        // 执行搜索
        // Measure the time taken to execute the search
        QElapsedTimer searchTimer;
        searchTimer.start();

        int32_t maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : reader->numDocs();
        TopDocsPtr topDocs = searcher->search(m_currentQuery, maxResults);
        Collection<ScoreDocPtr> scoreDocs = topDocs->scoreDocs;
        qInfo() << "Content search execution time:" << searchTimer.elapsed() << "ms";

        // 处理搜索结果
        processSearchResults(searcher, scoreDocs);

    } catch (const LuceneException &e) {
        qWarning() << "Lucene search exception:" << QString::fromStdWString(e.getError());
        emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
    } catch (const std::exception &e) {
        qWarning() << "Standard exception:" << e.what();
        emit errorOccurred(SearchError(ContentSearchErrorCode::ContentIndexException));
    }
}

void ContentIndexedStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
