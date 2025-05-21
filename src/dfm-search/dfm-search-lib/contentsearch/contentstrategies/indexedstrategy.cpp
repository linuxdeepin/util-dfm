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
#include "utils/lucenequeryutils.h"

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
        m_keywords.clear();
        ContentOptionsAPI optAPI(m_options);   // Use the member m_options
        bool mixedAndEnabled = optAPI.isFilenameContentMixedAndSearchEnabled();

        Lucene::QueryParserPtr contentsParser = newLucene<Lucene::QueryParser>(
                Lucene::LuceneVersion::LUCENE_CURRENT,
                L"contents",
                analyzer);

        if (query.type() == SearchQuery::Type::Simple) {
            return buildSimpleContentsQuery(query, contentsParser);
        } else if (query.type() == SearchQuery::Type::Boolean) {
            if (query.subQueries().isEmpty()) {
                // For an empty boolean query, match nothing.
                return newLucene<Lucene::BooleanQuery>();
            }

            // Determine which logic path to take for boolean queries
            if (mixedAndEnabled && query.booleanOperator() == SearchQuery::BooleanOperator::AND) {
                // New "advanced" AND logic for contents/filename
                return buildAdvancedAndQuery(query, contentsParser, analyzer);
            } else {
                // "Standard" contents-only logic for:
                // 1. OR queries (regardless of mixedAndEnabled value).
                // 2. AND queries when mixedAndEnabled is false.
                return buildStandardBooleanContentsQuery(query, contentsParser);
            }
        } else {
            qWarning() << "Unknown SearchQuery type encountered.";
            return newLucene<Lucene::BooleanQuery>();   // Should not happen
        }

    } catch (const Lucene::LuceneException &e) {
        qWarning() << "Error building Lucene query:" << QString::fromStdWString(e.getError());
        return nullptr;
    } catch (const std::exception &e) {
        qWarning() << "Standard exception building Lucene query:" << e.what();
        return nullptr;
    }
}

QueryPtr ContentIndexedStrategy::buildAdvancedAndQuery(const SearchQuery &query, const Lucene::QueryParserPtr &contentsParser, const Lucene::AnalyzerPtr &analyzer)
{
    // This method implements the new "mixed" AND logic.
    // It requires its own filenameParser.
    Lucene::QueryParserPtr filenameParser = newLucene<Lucene::QueryParser>(
            Lucene::LuceneVersion::LUCENE_CURRENT,
            L"filename",
            analyzer);

    Lucene::BooleanQueryPtr overallQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr mainAndClausesQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr allFilenamesQuery = newLucene<Lucene::BooleanQuery>();
    bool hasValidKeywords = false;

    for (const auto &subQuery : query.subQueries()) {
        m_keywords.append(subQuery.keyword());
        if (subQuery.keyword().isEmpty()) {
            continue;   // Skip empty keywords
        }
        hasValidKeywords = true;

        // 使用 LuceneQueryUtils 处理特殊字符
        Lucene::String processedKeyword = LuceneQueryUtils::processQueryString(subQuery.keyword(), false);
        Lucene::QueryPtr contentsTermQuery = contentsParser->parse(processedKeyword);
        Lucene::QueryPtr filenameTermQuery = filenameParser->parse(processedKeyword);

        // Build (contents:keyword OR filename:keyword)
        Lucene::BooleanQueryPtr combinedTermQuery = newLucene<Lucene::BooleanQuery>();
        combinedTermQuery->add(contentsTermQuery, Lucene::BooleanClause::SHOULD);
        combinedTermQuery->add(filenameTermQuery, Lucene::BooleanClause::SHOULD);

        mainAndClausesQuery->add(combinedTermQuery, Lucene::BooleanClause::MUST);
        allFilenamesQuery->add(filenameTermQuery, Lucene::BooleanClause::MUST);
    }

    if (!hasValidKeywords) {   // All subQuery keywords were empty
        return newLucene<Lucene::BooleanQuery>();   // Matches nothing
    }

    // Final query: ( (c:k1 OR f:k1) AND ... ) AND NOT (f:k1 AND f:k2 ...)
    overallQuery->add(mainAndClausesQuery, Lucene::BooleanClause::MUST);
    overallQuery->add(allFilenamesQuery, Lucene::BooleanClause::MUST_NOT);
    return overallQuery;
}

QueryPtr ContentIndexedStrategy::buildStandardBooleanContentsQuery(const SearchQuery &query, const Lucene::QueryParserPtr &contentsParser)
{
    // This method implements the "original" boolean logic, searching only "contents".
    Lucene::BooleanQueryPtr booleanQuery = newLucene<Lucene::BooleanQuery>();

    for (const auto &subQuery : query.subQueries()) {
        m_keywords.append(subQuery.keyword());
        if (subQuery.keyword().isEmpty()) {
            continue;   // Skip empty keywords
        }

        // 使用 LuceneQueryUtils 处理特殊字符
        Lucene::QueryPtr termQuery = contentsParser->parse(LuceneQueryUtils::processQueryString(subQuery.keyword(), false));
        booleanQuery->add(termQuery,
                          query.booleanOperator() == SearchQuery::BooleanOperator::AND ? Lucene::BooleanClause::MUST : Lucene::BooleanClause::SHOULD);
    }

    return booleanQuery;
}

QueryPtr ContentIndexedStrategy::buildSimpleContentsQuery(const SearchQuery &query, const Lucene::QueryParserPtr &contentsParser)
{
    m_keywords.append(query.keyword());
    if (query.keyword().isEmpty()) {
        return newLucene<Lucene::BooleanQuery>();   // Match nothing for empty keyword
    }
    // 使用 LuceneQueryUtils 处理特殊字符
    return contentsParser->parse(LuceneQueryUtils::processQueryString(query.keyword(), false));
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
            if (!scoreDoc) {
                qWarning() << "Null ScoreDoc encountered at index" << i;
                continue;
            }

            // Defensive check: verify document ID is valid
            if (scoreDoc->doc < 0) {
                qWarning() << "Invalid document ID:" << scoreDoc->doc;
                continue;
            }

            // Safely retrieve document (could throw if index is corrupted)
            Lucene::DocumentPtr doc;
            try {
                doc = searcher->doc(scoreDoc->doc);
                if (!doc) {
                    qWarning() << "Failed to retrieve document at index:" << scoreDoc->doc;
                    continue;
                }
            } catch (const Lucene::LuceneException &e) {
                qWarning() << "Exception while retrieving document:" << QString::fromStdWString(e.getError());
                continue;
            } catch (const std::exception &e) {
                qWarning() << "Standard exception while retrieving document:" << e.what();
                continue;
            }

            // Safely get path
            Lucene::String pathField;
            try {
                pathField = doc->get(L"path");
                if (pathField.empty()) {
                    qWarning() << "Document missing path field at index:" << scoreDoc->doc;
                    continue;
                }
            } catch (const std::exception &e) {
                qWarning() << "Exception retrieving path field:" << e.what();
                continue;
            }

            QString path = QString::fromStdWString(pathField);

            if (!path.startsWith(searchPath)) {
                continue;
            }

            if (std::any_of(searchExcludedPaths.cbegin(), searchExcludedPaths.cend(),
                            [&path](const auto &excluded) { return path.startsWith(excluded); })) {
                continue;
            }

            // Safely check hidden status
            bool isHidden = false;
            if (Q_LIKELY(!m_options.includeHidden())) {
                try {
                    Lucene::String hiddenField = doc->get(L"is_hidden");
                    if (!hiddenField.empty() && QString::fromStdWString(hiddenField).toLower() == "y") {
                        continue;
                    }
                } catch (const std::exception &e) {
                    qWarning() << "Exception retrieving is_hidden field:" << e.what();
                    // Default to visible if field can't be read
                }
            }

            // 创建搜索结果
            SearchResult result(path);

            // 设置内容结果
            ContentResultAPI resultApi(result);

            // 使用ContentHighlighter命名空间进行高亮
            if (enableRetrieval) {
                try {
                    // Safely get contents with null check
                    Lucene::String contentField = doc->get(L"contents");
                    if (!contentField.empty()) {
                        const QString content = QString::fromStdWString(contentField);
                        const QString highlightedContent = ContentHighlighter::customHighlight(m_keywords, content, previewLen, enableHTML);
                        resultApi.setHighlightedContent(highlightedContent);
                    }
                } catch (const Lucene::LuceneException &e) {
                    qWarning() << "Exception retrieving content field:" << QString::fromStdWString(e.getError());
                    // Continue without content highlight
                } catch (const std::exception &e) {
                    qWarning() << "Standard exception retrieving content field:" << e.what();
                    // Continue without content highlight
                }
            }
            
            // 添加到结果集合
            m_results.append(result);

            // 实时发送结果
            if (Q_UNLIKELY(m_options.resultFoundEnabled()))
                emit resultFound(result);

        } catch (const Lucene::LuceneException &e) {
            qWarning() << "Error processing result:" << QString::fromStdWString(e.getError());
            continue;
        } catch (const std::exception &e) {
            qWarning() << "Standard exception:" << e.what();
            continue;
        } catch (...) {
            qWarning() << "Unknown exception during result processing";
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
