// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "indexedstrategy.h"

#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QElapsedTimer>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/QueryParser.h>
#include <lucene++/BooleanQuery.h>
#include <lucene++/QueryWrapperFilter.h>
#include <lucene++/WildcardQuery.h>

#include <dfm-search/field_names.h>
#include <dfm-search/timerangefilter.h>

#include "3rdparty/fulltext/chineseanalyzer.h"
#include "utils/cancellablecollector.h"
#include "utils/lucenequeryutils.h"
#include "utils/searchutility.h"
#include "utils/lucene_cancellation_compat.h"
#include "utils/timerangeutils.h"

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

OcrTextIndexedStrategy::OcrTextIndexedStrategy(const SearchOptions &options, QObject *parent)
    : OcrTextBaseStrategy(options, parent)
{
    initializeIndexing();
}

OcrTextIndexedStrategy::~OcrTextIndexedStrategy() = default;

void OcrTextIndexedStrategy::initializeIndexing()
{
    // Get OCR text index directory
    m_indexDir = Global::ocrTextIndexDirectory();

    // Check if index directory exists
    if (!QDir(m_indexDir).exists()) {
        qWarning() << "OCR text index directory does not exist:" << m_indexDir;
    }
}

void OcrTextIndexedStrategy::search(const SearchQuery &query)
{
    m_cancelled.store(false);
    m_results.clear();

    try {
        // Perform OCR text index search
        performOcrTextSearch(query);
    } catch (const std::exception &e) {
        qWarning() << "OCR Text Index Search Exception:" << e.what();
        emit errorOccurred(SearchError(OcrTextSearchErrorCode::OcrTextIndexException));
    }
}

Lucene::QueryPtr OcrTextIndexedStrategy::buildLuceneQuery(const SearchQuery &query, const Lucene::AnalyzerPtr &analyzer, const QString &searchPath)
{
    try {
        m_keywords.clear();
        OcrTextOptionsAPI optAPI(m_options);
        bool mixedAndEnabled = optAPI.isFilenameOcrContentMixedAndSearchEnabled();

        Lucene::QueryParserPtr ocrContentsParser = newLucene<Lucene::QueryParser>(
                Lucene::LuceneVersion::LUCENE_CURRENT,
                LuceneFieldNames::OcrText::kOcrContents,
                analyzer);

        Lucene::QueryPtr mainQuery;
        if (query.type() == SearchQuery::Type::Simple) {
            mainQuery = buildSimpleOcrContentsQuery(query, ocrContentsParser);
        } else if (query.type() == SearchQuery::Type::Boolean) {
            if (query.subQueries().isEmpty()) {
                // For an empty boolean query, match nothing.
                mainQuery = newLucene<Lucene::BooleanQuery>();
            } else {
                // Determine which logic path to take for boolean queries
                if (mixedAndEnabled && query.booleanOperator() == SearchQuery::BooleanOperator::AND) {
                    // New "advanced" AND logic for ocr_contents/filename
                    mainQuery = buildAdvancedAndQuery(query, ocrContentsParser, analyzer);
                } else {
                    // "Standard" ocr_contents-only logic
                    mainQuery = buildStandardBooleanOcrContentsQuery(query, ocrContentsParser);
                }
            }
        } else {
            qWarning() << "Unknown SearchQuery type encountered.";
            mainQuery = newLucene<Lucene::BooleanQuery>();   // Should not happen
        }

        // Add path prefix query optimization
        if (mainQuery && SearchUtility::isOcrTextIndexAncestorPathsSupported()
            && SearchUtility::shouldUsePathPrefixQuery(searchPath)) {
            QueryPtr pathPrefixQuery = LuceneQueryUtils::buildPathPrefixQuery(searchPath,
                                                                              QString::fromWCharArray(LuceneFieldNames::OcrText::kAncestorPaths));
            if (pathPrefixQuery) {
                BooleanQueryPtr finalQuery = newLucene<BooleanQuery>();
                finalQuery->add(mainQuery, BooleanClause::MUST);
                finalQuery->add(pathPrefixQuery, BooleanClause::MUST);
                qInfo() << "Using path prefix query for OCR text search optimization:" << searchPath;
                mainQuery = finalQuery;
            }
        }

        // Add time range filter query
        if (m_options.hasTimeRangeFilter()) {
            TimeRangeFilter filter = m_options.timeRangeFilter();
            auto [start, end] = filter.resolveTimeRange();

            qint64 startEpoch = TimeRangeUtils::toEpochSecs(start);
            qint64 endEpoch = TimeRangeUtils::toEpochSecs(end);

            const wchar_t *fieldName = (filter.timeField() == TimeField::BirthTime)
                    ? LuceneFieldNames::OcrText::kBirthTime
                    : LuceneFieldNames::OcrText::kModifyTime;

            QueryPtr timeQuery = TimeRangeUtils::buildNumericRangeQuery(
                    fieldName, startEpoch, endEpoch,
                    filter.includeLower(), filter.includeUpper());

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

        return mainQuery;

    } catch (const Lucene::LuceneException &e) {
        qWarning() << "Error building Lucene query:" << QString::fromStdWString(e.getError());
        return nullptr;
    } catch (const std::exception &e) {
        qWarning() << "Standard exception building Lucene query:" << e.what();
        return nullptr;
    }
}

QueryPtr OcrTextIndexedStrategy::buildAdvancedAndQuery(const SearchQuery &query, const Lucene::QueryParserPtr &ocrContentsParser, const Lucene::AnalyzerPtr &analyzer)
{
    // This method implements the "mixed" AND logic similar to content search.
    // It requires its own filenameParser.
    Lucene::QueryParserPtr filenameParser = newLucene<Lucene::QueryParser>(
            Lucene::LuceneVersion::LUCENE_CURRENT,
            LuceneFieldNames::OcrText::kFilename,
            analyzer);

    Lucene::BooleanQueryPtr overallQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr mainAndClausesQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr allOcrContentsQuery = newLucene<Lucene::BooleanQuery>();
    Lucene::BooleanQueryPtr allFilenamesQuery = newLucene<Lucene::BooleanQuery>();
    bool hasValidKeywords = false;

    for (const auto &subQuery : query.subQueries()) {
        m_keywords.append(subQuery.keyword());
        if (subQuery.keyword().isEmpty()) {
            continue;   // Skip empty keywords
        }
        hasValidKeywords = true;

        // Use LuceneQueryUtils to process special characters
        Lucene::String processedKeyword = LuceneQueryUtils::processQueryString(subQuery.keyword(), false);
        Lucene::QueryPtr ocrContentsTermQuery = ocrContentsParser->parse(processedKeyword);
        Lucene::QueryPtr filenameTermQuery = filenameParser->parse(processedKeyword);

        // Build (ocr_contents:keyword OR filename:keyword)
        Lucene::BooleanQueryPtr combinedTermQuery = newLucene<Lucene::BooleanQuery>();
        combinedTermQuery->add(ocrContentsTermQuery, Lucene::BooleanClause::SHOULD);
        combinedTermQuery->add(filenameTermQuery, Lucene::BooleanClause::SHOULD);

        mainAndClausesQuery->add(combinedTermQuery, Lucene::BooleanClause::MUST);
        allOcrContentsQuery->add(ocrContentsTermQuery, Lucene::BooleanClause::MUST);
        allFilenamesQuery->add(filenameTermQuery, Lucene::BooleanClause::MUST);
    }

    if (!hasValidKeywords) {   // All subQuery keywords were empty
        qWarning() << "No valid keywords found in advanced AND query";
        return newLucene<Lucene::BooleanQuery>();   // Matches nothing
    }

    // Exclude pure filename-only matches
    // Final query: ( (ocr:k1 OR f:k1) AND ... ) AND NOT (f:k1 AND f:k2 ... AND NOT (ocr:k1 AND ocr:k2 ...))
    Lucene::BooleanQueryPtr pureFilenameQuery = newLucene<Lucene::BooleanQuery>();
    pureFilenameQuery->add(allFilenamesQuery, Lucene::BooleanClause::MUST);
    pureFilenameQuery->add(allOcrContentsQuery, Lucene::BooleanClause::MUST_NOT);

    overallQuery->add(mainAndClausesQuery, Lucene::BooleanClause::MUST);
    overallQuery->add(pureFilenameQuery, Lucene::BooleanClause::MUST_NOT);

    return overallQuery;
}

QueryPtr OcrTextIndexedStrategy::buildStandardBooleanOcrContentsQuery(const SearchQuery &query, const Lucene::QueryParserPtr &ocrContentsParser)
{
    // This method implements the "original" boolean logic, searching only "ocr_contents".
    Lucene::BooleanQueryPtr booleanQuery = newLucene<Lucene::BooleanQuery>();

    for (const auto &subQuery : query.subQueries()) {
        m_keywords.append(subQuery.keyword());
        if (subQuery.keyword().isEmpty()) {
            continue;   // Skip empty keywords
        }

        // Use LuceneQueryUtils to process special characters
        Lucene::QueryPtr termQuery = ocrContentsParser->parse(LuceneQueryUtils::processQueryString(subQuery.keyword(), false));
        booleanQuery->add(termQuery,
                          query.booleanOperator() == SearchQuery::BooleanOperator::AND ? Lucene::BooleanClause::MUST : Lucene::BooleanClause::SHOULD);
    }

    return booleanQuery;
}

QueryPtr OcrTextIndexedStrategy::buildSimpleOcrContentsQuery(const SearchQuery &query, const Lucene::QueryParserPtr &ocrContentsParser)
{
    m_keywords.append(query.keyword());
    if (query.keyword().isEmpty()) {
        return newLucene<Lucene::BooleanQuery>();   // Match nothing for empty keyword
    }
    // Use LuceneQueryUtils to process special characters
    return ocrContentsParser->parse(LuceneQueryUtils::processQueryString(query.keyword(), false));
}

void OcrTextIndexedStrategy::processSearchResults(const Lucene::IndexSearcherPtr &searcher,
                                                  const Lucene::Collection<Lucene::ScoreDocPtr> &scoreDocs)
{
    // Measure the time taken to process search results
    QElapsedTimer resultTimer;
    resultTimer.start();

    QString searchPath = m_options.searchPath();
    const QStringList &searchExcludedPaths = m_options.searchExcludedPaths();
    auto docsSize = scoreDocs.size();

    for (int32_t i = 0; i < docsSize; ++i) {
        if (m_cancelled.load()) {
            qInfo() << "OCR text search cancelled";
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
                pathField = doc->get(LuceneFieldNames::OcrText::kPath);
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
            if (Q_LIKELY(!m_options.includeHidden())) {
                try {
                    Lucene::String hiddenField = doc->get(LuceneFieldNames::OcrText::kIsHidden);
                    if (!hiddenField.empty() && QString::fromStdWString(hiddenField).toLower() == "y") {
                        continue;
                    }
                } catch (const std::exception &e) {
                    qWarning() << "Exception retrieving is_hidden field:" << e.what();
                    // Default to visible if field can't be read
                }
            }

            // Create search result
            SearchResult result(path);

            // Add to result collection
            m_results.append(result);

            // Real-time result emission
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

    qInfo() << "OCR text result processing time:" << resultTimer.elapsed() << "ms";
    emit searchFinished(m_results);
}

void OcrTextIndexedStrategy::performOcrTextSearch(const SearchQuery &query)
{
    // RAII guard: automatically manage cancellation flag lifecycle
    SearchCancellationGuard guard(&m_cancelled);

    try {
        // Get index directory
        FSDirectoryPtr directory = FSDirectory::open(m_indexDir.toStdWString());
        if (!directory) {
            qWarning() << "Failed to open OCR text index directory:" << m_indexDir;
            emit errorOccurred(SearchError(OcrTextSearchErrorCode::OcrTextIndexNotFound));
            return;
        }

        // Get index reader
        IndexReaderPtr reader = IndexReader::open(directory, true);
        if (!reader || reader->numDocs() == 0) {
            qWarning() << "OCR text index is empty or cannot be opened";
            emit errorOccurred(SearchError(OcrTextSearchErrorCode::OcrTextIndexNotFound));
            return;
        }

        // Create searcher
        IndexSearcherPtr searcher = newLucene<IndexSearcher>(reader);

        // Create analyzer (reuse ChineseAnalyzer for OCR text)
        AnalyzerPtr analyzer = newLucene<ChineseAnalyzer>();

        // Build query
        m_currentQuery = buildLuceneQuery(query, analyzer, m_options.searchPath());
        if (!m_currentQuery) {
            qWarning() << "Failed to build Lucene query for OCR text search";
            emit errorOccurred(SearchError(OcrTextSearchErrorCode::OcrTextIndexException));
            return;
        }

        // Execute search
        QElapsedTimer searchTimer;
        searchTimer.start();

        int32_t maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : reader->numDocs();

        // Use custom CancellableCollector for interruptible search
        Collection<ScoreDocPtr> scoreDocs;
        try {
            // Create cancellable collector
            boost::shared_ptr<CancellableCollector> collector = newLucene<CancellableCollector>(&m_cancelled, maxResults);

            // Execute search with custom collector
            qInfo() << "OCR text search execution start:" << query.keyword();
            searcher->search(m_currentQuery, collector);
            // Get collected documents
            scoreDocs = collector->getScoreDocs();

            qInfo() << "OCR text search execution time:" << searchTimer.elapsed() << "ms"
                    << "Total hits:" << collector->getTotalHits()
                    << "Collected:" << scoreDocs.size()
                    << "Keyword:" << query.keyword()
                    << "Cancelled" << m_cancelled.load();
        } catch (const SearchCancelledException &e) {
            qInfo() << "OCR text search cancelled during execution";
            emit searchFinished(m_results);
            return;
        } catch (const RuntimeException &e) {
#if LUCENE_HAS_SEARCH_CANCELLATION
            // Check if this is a cancellation exception thrown in ExactPhraseScorer::phraseFreq()
            QString errorMsg = QString::fromStdWString(e.getError());
            if (errorMsg.contains("cancelled", Qt::CaseInsensitive)) {
                qInfo() << "OCR text search cancelled in phraseFreq():" << errorMsg;
                emit searchFinished(m_results);
                return;
            }
#endif
            // Other runtime exceptions, rethrow
            throw;
        }

        // Process search results
        processSearchResults(searcher, scoreDocs);
    } catch (const LuceneException &e) {
        qWarning() << "Lucene search exception:" << QString::fromStdWString(e.getError());
        emit errorOccurred(SearchError(OcrTextSearchErrorCode::OcrTextIndexException));
    } catch (const std::exception &e) {
        qWarning() << "Standard exception:" << e.what();
        emit errorOccurred(SearchError(OcrTextSearchErrorCode::OcrTextIndexException));
    }
}

void OcrTextIndexedStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
