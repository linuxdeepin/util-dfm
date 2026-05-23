// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
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
#include <lucene++/BooleanQuery.h>
#include <lucene++/QueryWrapperFilter.h>
#include <lucene++/WildcardQuery.h>
#include <lucene++/MapFieldSelector.h>

#include <dfm-search/field_names.h>
#include <dfm-search/timerangefilter.h>

#include "utils/cancellablecollector.h"
#include "utils/contenthighlighter.h"
#include "utils/lucenequeryutils.h"
#include "utils/lucene_cancellation_compat.h"
#include "utils/timerangeutils.h"

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

Lucene::QueryPtr ContentIndexedStrategy::buildLuceneQuery(const SearchQuery &query)
{
    try {
        m_keywords.clear();
        ContentOptionsAPI optAPI(m_options);   // Use the member m_options
        bool mixedAndEnabled = optAPI.isFilenameContentMixedAndSearchEnabled();

        Lucene::QueryPtr mainQuery;
        if (query.type() == SearchQuery::Type::Simple) {
            mainQuery = buildSimpleContentsQuery(query);
        } else if (query.type() == SearchQuery::Type::Boolean) {
            if (query.subQueries().isEmpty()) {
                // For an empty boolean query, match nothing.
                mainQuery = newLucene<Lucene::BooleanQuery>();
            } else {
                // Determine which logic path to take for boolean queries
                if (mixedAndEnabled && query.booleanOperator() == SearchQuery::BooleanOperator::AND) {
                    // New "advanced" AND logic for contents/filename
                    mainQuery = buildAdvancedAndQuery(query);
                } else {
                    // "Standard" contents-only logic for:
                    // 1. OR queries (regardless of mixedAndEnabled value).
                    // 2. AND queries when mixedAndEnabled is false.
                    mainQuery = buildStandardBooleanContentsQuery(query);
                }
            }
        } else {
            qWarning() << "Unknown SearchQuery type encountered.";
            mainQuery = newLucene<Lucene::BooleanQuery>();   // Should not happen
        }

        // Add filename keyword query (before filters, so it replaces empty content query correctly)
        QString filenameKw = optAPI.filenameKeyword();
        if (!filenameKw.isEmpty()) {
            Lucene::QueryPtr filenameQuery = LuceneQueryUtils::buildNGramSearchQuery(
                    QString::fromWCharArray(LuceneFieldNames::Content::kFilename),
                    filenameKw);

            if (filenameQuery) {
                // Check if content keywords are effectively empty
                bool noContentKeywords = (query.type() == SearchQuery::Type::Simple)
                        ? query.keyword().isEmpty()
                        : (query.subQueries().isEmpty()
                           || std::all_of(query.subQueries().cbegin(), query.subQueries().cend(),
                                          [](const auto &sq) { return sq.keyword().isEmpty(); }));

                if (noContentKeywords) {
                    // Filename-only search: replace empty content query with filename query
                    mainQuery = filenameQuery;
                } else if (mainQuery) {
                    // Both content and filename: AND combination
                    BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
                    finalQuery->add(mainQuery, BooleanClause::MUST);
                    finalQuery->add(filenameQuery, BooleanClause::MUST);
                    mainQuery = finalQuery;
                }
                m_keywords.append(filenameKw);
            }
        }

        // Add path prefix query optimization
        QStringList searchPathsList = m_options.searchPaths();
        if (mainQuery) {
            QueryPtr pathPrefixQuery = LuceneQueryUtils::buildMultiPathPrefixQuery(
                    searchPathsList,
                    QString::fromWCharArray(LuceneFieldNames::Content::kAncestorPaths));
            if (pathPrefixQuery) {
                BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
                finalQuery->add(mainQuery, BooleanClause::MUST);
                finalQuery->add(pathPrefixQuery, BooleanClause::MUST);
                qInfo() << "Using multi-path prefix query for content search optimization:" << searchPathsList;
                mainQuery = finalQuery;
            }
        }

        // Add excluded paths filter (pushed down to query layer to avoid per-doc filtering)
        if (mainQuery) {
            const QStringList &excludedPaths = m_options.searchExcludedPaths();
            if (!excludedPaths.isEmpty()) {
                QueryPtr excludedQuery = LuceneQueryUtils::buildMultiPathPrefixQuery(
                        excludedPaths,
                        QString::fromWCharArray(LuceneFieldNames::Content::kAncestorPaths));
                if (excludedQuery) {
                    BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
                    finalQuery->add(mainQuery, BooleanClause::MUST);
                    finalQuery->add(excludedQuery, BooleanClause::MUST_NOT);
                    mainQuery = finalQuery;
                }
            }
        }

        // Add hidden file filter (pushed down to query layer)
        if (mainQuery && !m_options.includeHidden()) {
            QueryPtr hiddenQuery = Lucene::newLucene<Lucene::TermQuery>(
                    Lucene::newLucene<Lucene::Term>(
                            LuceneFieldNames::Content::kIsHidden,
                            L"Y"));
            BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
            finalQuery->add(mainQuery, BooleanClause::MUST);
            finalQuery->add(hiddenQuery, BooleanClause::MUST_NOT);
            mainQuery = finalQuery;
        }

        // Add time range filter query
        if (m_options.hasTimeRangeFilter()) {
            TimeRangeFilter filter = m_options.timeRangeFilter();
            QueryPtr timeQuery = TimeRangeUtils::buildTimeRangeFilterQuery(
                    filter,
                    LuceneFieldNames::Content::kBirthTime,
                    LuceneFieldNames::Content::kModifyTime);

            if (timeQuery) {
                if (mainQuery) {
                    BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
                    finalQuery->add(mainQuery, BooleanClause::MUST);
                    finalQuery->add(timeQuery, BooleanClause::MUST);
                    mainQuery = finalQuery;
                } else {
                    // Time filter alone is a valid query
                    mainQuery = timeQuery;
                }
            }
        }

        // Add file size range filter query
        if (m_options.hasSizeRangeFilter()) {
            SizeRangeFilter sizeFilter = m_options.sizeRangeFilter();
            QueryPtr sizeQuery = TimeRangeUtils::buildNumericRangeQuery(
                    LuceneFieldNames::Content::kFileSize,
                    sizeFilter.minSize(), sizeFilter.maxSize(),
                    sizeFilter.includeLower(), sizeFilter.includeUpper());

            if (sizeQuery) {
                if (mainQuery) {
                    BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
                    finalQuery->add(mainQuery, BooleanClause::MUST);
                    finalQuery->add(sizeQuery, BooleanClause::MUST);
                    mainQuery = finalQuery;
                } else {
                    // Size filter alone is a valid query
                    mainQuery = sizeQuery;
                }
            }
        }

        return mainQuery;

    } catch (const Lucene::LuceneException &e) {
        qWarning() << "Error building Lucene query:" << QString::fromStdWString(e.getError());
        return nullptr;
    } catch (const std::exception &e) {
        qWarning() << "Standard exception building Lucene query:" << e.what();
        return nullptr;
    }
}

QueryPtr ContentIndexedStrategy::buildAdvancedAndQuery(const SearchQuery &query)
{
    // This method implements the new "mixed" AND logic.
    Lucene::BooleanQueryPtr overallQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr mainAndClausesQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr allContentsQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr allFilenamesQuery = newLucene<Lucene::BooleanQuery>();
    bool hasValidKeywords = false;

    for (const auto &subQuery : query.subQueries()) {
        m_keywords.append(subQuery.keyword());
        if (subQuery.keyword().isEmpty()) {
            continue;   // Skip empty keywords
        }
        hasValidKeywords = true;

        Lucene::QueryPtr contentsTermQuery = LuceneQueryUtils::buildNGramSearchQuery(
                QString::fromWCharArray(LuceneFieldNames::Content::kContents),
                subQuery.keyword());
        Lucene::QueryPtr filenameTermQuery = LuceneQueryUtils::buildNGramSearchQuery(
                QString::fromWCharArray(LuceneFieldNames::Content::kFilename),
                subQuery.keyword());

        // Build (contents:keyword OR filename:keyword)
        Lucene::BooleanQueryPtr combinedTermQuery = newLucene<Lucene::BooleanQuery>();
        combinedTermQuery->add(contentsTermQuery, Lucene::BooleanClause::SHOULD);
        combinedTermQuery->add(filenameTermQuery, Lucene::BooleanClause::SHOULD);

        mainAndClausesQuery->add(combinedTermQuery, Lucene::BooleanClause::MUST);
        allContentsQuery->add(contentsTermQuery, Lucene::BooleanClause::MUST);
        allFilenamesQuery->add(filenameTermQuery, Lucene::BooleanClause::MUST);
    }

    if (!hasValidKeywords) {   // All subQuery keywords were empty
        qWarning() << "No valid keywords found in advanced AND query";
        return newLucene<Lucene::BooleanQuery>();   // Matches nothing
    }

    // New logic: Include results that match content OR exclude pure filename-only matches
    // Final query: ( (c:k1 OR f:k1) AND ... ) AND NOT (f:k1 AND f:k2 ... AND NOT (c:k1 AND c:k2 ...))
    // This means: exclude documents that match all keywords in filename but don't match all keywords in content
    Lucene::BooleanQueryPtr pureFilenameQuery = newLucene<Lucene::BooleanQuery>();
    pureFilenameQuery->add(allFilenamesQuery, Lucene::BooleanClause::MUST);
    pureFilenameQuery->add(allContentsQuery, Lucene::BooleanClause::MUST_NOT);

    overallQuery->add(mainAndClausesQuery, Lucene::BooleanClause::MUST);
    overallQuery->add(pureFilenameQuery, Lucene::BooleanClause::MUST_NOT);

    return overallQuery;
}

QueryPtr ContentIndexedStrategy::buildStandardBooleanContentsQuery(const SearchQuery &query)
{
    // This method implements the "original" boolean logic, searching only "contents".
    Lucene::BooleanQueryPtr booleanQuery = newLucene<Lucene::BooleanQuery>();

    for (const auto &subQuery : query.subQueries()) {
        m_keywords.append(subQuery.keyword());
        if (subQuery.keyword().isEmpty()) {
            continue;   // Skip empty keywords
        }

        Lucene::QueryPtr termQuery = LuceneQueryUtils::buildNGramSearchQuery(
                QString::fromWCharArray(LuceneFieldNames::Content::kContents),
                subQuery.keyword());
        booleanQuery->add(termQuery,
                          query.booleanOperator() == SearchQuery::BooleanOperator::AND ? Lucene::BooleanClause::MUST : Lucene::BooleanClause::SHOULD);
    }

    return booleanQuery;
}

QueryPtr ContentIndexedStrategy::buildSimpleContentsQuery(const SearchQuery &query)
{
    m_keywords.append(query.keyword());
    if (query.keyword().isEmpty()) {
        return newLucene<Lucene::BooleanQuery>();   // Match nothing for empty keyword
    }
    return LuceneQueryUtils::buildNGramSearchQuery(
            QString::fromWCharArray(LuceneFieldNames::Content::kContents),
            query.keyword());
}

void ContentIndexedStrategy::processSearchResults(const Lucene::IndexSearcherPtr &searcher,
                                                  const Lucene::Collection<Lucene::ScoreDocPtr> &scoreDocs)
{
    // Measure the time taken to process search results
    QElapsedTimer resultTimer;
    resultTimer.start();

    auto docsSize = scoreDocs.size();

    ContentOptionsAPI optAPI(m_options);
    bool enableHTML = optAPI.isSearchResultHighlightEnabled();
    int previewLen = optAPI.maxPreviewLength() > 0 ? optAPI.maxPreviewLength() : 50;
    bool enableRetrieval = optAPI.isFullTextRetrievalEnabled();
    bool detailedResults = m_options.detailedResultsEnabled();

    // Build field selector to avoid loading the large 'contents' field when not needed.
    // The contents field stores full document text and loading it for every result
    // (even when only path is needed) causes significant disk I/O overhead.
    Lucene::Collection<Lucene::String> fieldsToLoad = Lucene::Collection<Lucene::String>::newInstance();
    if (enableRetrieval) {
        fieldsToLoad.add(LuceneFieldNames::Content::kContents);
    }
    fieldsToLoad.add(LuceneFieldNames::Content::kPath);
    if (Q_UNLIKELY(detailedResults)) {
        fieldsToLoad.add(LuceneFieldNames::Content::kFilename);
        fieldsToLoad.add(LuceneFieldNames::Content::kIsHidden);
        fieldsToLoad.add(LuceneFieldNames::Content::kModifyTime);
        fieldsToLoad.add(LuceneFieldNames::Content::kBirthTime);
        fieldsToLoad.add(LuceneFieldNames::Content::kFileSize);
    }
    Lucene::FieldSelectorPtr fieldSelector = newLucene<Lucene::MapFieldSelector>(fieldsToLoad);

    // Pre-allocate to avoid reallocation during append
    m_results.reserve(m_results.size() + static_cast<int>(docsSize));

    for (int32_t i = 0; i < docsSize; ++i) {
        if (m_cancelled.load()) {
            qInfo() << "Content search cancelled";
            break;
        }

        try {
            Lucene::ScoreDocPtr scoreDoc = scoreDocs[i];
            if (!scoreDoc || scoreDoc->doc < 0) {
                qWarning() << "Invalid ScoreDoc at index" << i;
                continue;
            }

            Lucene::DocumentPtr doc;
            try {
                doc = searcher->doc(scoreDoc->doc, fieldSelector);
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

            // Path filtering, hidden file exclusion — handled at query layer
            Lucene::String pathField = doc->get(LuceneFieldNames::Content::kPath);
            if (pathField.empty()) {
                qWarning() << "Document missing path field at index:" << scoreDoc->doc;
                continue;
            }

            SearchResult result(QString::fromStdWString(pathField));
            ContentResultAPI resultApi(result);

            if (enableRetrieval) {
                try {
                    Lucene::String contentField = doc->get(LuceneFieldNames::Content::kContents);
                    if (!contentField.empty()) {
                        const QString content = QString::fromStdWString(contentField);
                        const QString highlightedContent = ContentHighlighter::customHighlight(m_keywords, content, previewLen, enableHTML);
                        resultApi.setHighlightedContent(highlightedContent);
                    }
                } catch (const Lucene::LuceneException &e) {
                    qWarning() << "Exception retrieving content field:" << QString::fromStdWString(e.getError());
                } catch (const std::exception &e) {
                    qWarning() << "Standard exception retrieving content field:" << e.what();
                }
            }

            // 设置详细结果（如果启用）
            if (Q_UNLIKELY(detailedResults)) {
                // 文件名
                Lucene::String filenameField = doc->get(LuceneFieldNames::Content::kFilename);
                if (!filenameField.empty()) {
                    resultApi.setFilename(QString::fromStdWString(filenameField));
                }

                // 隐藏状态
                Lucene::String hiddenField = doc->get(LuceneFieldNames::Content::kIsHidden);
                if (!hiddenField.empty()) {
                    resultApi.setIsHidden(QString::fromStdWString(hiddenField).toLower() == "y");
                }

                // 修改时间
                Lucene::String modifyTimeField = doc->get(LuceneFieldNames::Content::kModifyTime);
                if (!modifyTimeField.empty()) {
                    bool ok = false;
                    qint64 timestamp = QString::fromStdWString(modifyTimeField).toLongLong(&ok);
                    if (ok && timestamp > 0) {
                        resultApi.setModifyTimestamp(timestamp);
                    }
                }

                // 创建时间
                Lucene::String birthTimeField = doc->get(LuceneFieldNames::Content::kBirthTime);
                if (!birthTimeField.empty()) {
                    bool ok = false;
                    qint64 timestamp = QString::fromStdWString(birthTimeField).toLongLong(&ok);
                    if (ok && timestamp > 0) {
                        resultApi.setBirthTimestamp(timestamp);
                    }
                }

                // 文件大小
                Lucene::String fileSizeField = doc->get(LuceneFieldNames::Content::kFileSize);
                if (!fileSizeField.empty()) {
                    bool ok = false;
                    qint64 fileSize = QString::fromStdWString(fileSizeField).toLongLong(&ok);
                    if (ok && fileSize > 0) {
                        resultApi.setFileSizeBytes(fileSize);
                    }
                }
            }

            // 添加到结果集合
            m_results.append(std::move(result));

            // 实时发送结果
            if (Q_UNLIKELY(m_options.resultFoundEnabled()))
                emit resultFound(m_results.last());

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
    // RAII 守护类：自动管理取消标志的生命周期
    // 构造时设置标志，析构时自动清理（即使发生异常）
    SearchCancellationGuard guard(&m_cancelled);

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

        // 构建查询
        m_currentQuery = buildLuceneQuery(query);
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

        // 使用自定义 CancellableCollector 实现可中断搜索
        Collection<ScoreDocPtr> scoreDocs;
        try {
            // 创建可取消的收集器
            boost::shared_ptr<CancellableCollector> collector = newLucene<CancellableCollector>(&m_cancelled, maxResults);

            // 执行搜索，使用自定义收集器
            qInfo() << "Content search execution start:" << query.keyword();
            searcher->search(m_currentQuery, collector);
            // 获取收集到的文档
            scoreDocs = collector->getScoreDocs();

            qInfo() << "Content search execution time:" << searchTimer.elapsed() << "ms"
                    << "Total hits:" << collector->getTotalHits()
                    << "Collected:" << scoreDocs.size()
                    << "Keyword:" << query.keyword()
                    << "Cancelled" << m_cancelled.load();
        } catch (const SearchCancelledException &e) {
            qInfo() << "Content search cancelled during execution";
            emit searchFinished(m_results);
            return;
        } catch (const RuntimeException &e) {
#if LUCENE_HAS_SEARCH_CANCELLATION
            // 仅在支持 SearchCancellation 时检查取消异常
            // 检查是否是在 ExactPhraseScorer::phraseFreq() 中抛出的取消异常
            QString errorMsg = QString::fromStdWString(e.getError());
            if (errorMsg.contains("cancelled", Qt::CaseInsensitive)) {
                qInfo() << "Content search cancelled in phraseFreq():" << errorMsg;
                emit searchFinished(m_results);
                return;
            }
#endif
            // 其他运行时异常，继续抛出
            throw;
        }

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
