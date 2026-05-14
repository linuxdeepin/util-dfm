// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTICSEARCHER_H
#define SEMANTICSEARCHER_H

#include <QObject>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searcherror.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

class SemanticSearcherData;

/**
 * @brief The SemanticSearcher class provides natural language based file search.
 *
 * This class parses natural language queries (e.g., "today's pdf documents")
 * into structured search conditions, then orchestrates parallel searches
 * across filename, content, and OCR indexes.
 *
 * Usage:
 * @code
 * SemanticSearcher *searcher = new SemanticSearcher(this);
 * connect(searcher, &SemanticSearcher::resultsFound, [](const SearchResultList &results) {
 *     for (const auto &r : results) {
 *         qDebug() << r.path();
 *     }
 * });
 * searcher->search("today's pdf documents");
 * @endcode
 */
class SemanticSearcher : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Construct a semantic searcher
     * @param parent Parent QObject
     */
    explicit SemanticSearcher(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SemanticSearcher() override;

    /**
     * @brief Get the current search status
     */
    SearchStatus status() const;

    /**
     * @brief Set the search timeout in seconds
     * @param seconds Timeout duration (default 60, 0 to disable)
     */
    void setSearchTimeout(int seconds);

    /**
     * @brief Get the search timeout in seconds
     */
    int searchTimeout() const;

    /**
     * @brief Perform a semantic search with natural language input
     * @param naturalLanguage The natural language query string
     */
    void search(const QString &naturalLanguage);

    /**
     * @brief Check if the input contains semantic intent beyond a plain keyword.
     *
     * Returns true if parsing the input reveals time constraints, size constraints,
     * file type filters, or location constraints. Returns false for plain keyword input.
     *
     * This allows callers to avoid unnecessary semantic search overhead when
     * the user is just typing a simple keyword.
     *
     * @param input The natural language query to check
     * @return true if the input contains semantic intent, false for plain keywords
     */
    bool isSemanticQuery(const QString &input) const;

    /**
     * @brief Perform a synchronous semantic search
     *
     * Blocks the calling thread until all search engines complete or timeout.
     * Uses QEventLoop internally, so it works from the GUI thread.
     * @param naturalLanguage The natural language query string
     * @return SearchResultExpected containing deduplicated results or an error
     */
    SearchResultExpected searchSync(const QString &naturalLanguage);

    /**
     * @brief Cancel the current search operation
     */
    void cancel();

Q_SIGNALS:
    /**
     * @brief Emitted when a search operation starts
     */
    void searchStarted();

    /**
     * @brief Emitted when search results are found
     * @param results The found search results
     */
    void resultsFound(const DFMSEARCH::SearchResultList &results);

    /**
     * @brief Emitted when the search status changes
     * @param status The new search status
     */
    void statusChanged(SearchStatus status);

    /**
     * @brief Emitted when a search operation completes
     * @param results The list of all search results (deduplicated)
     */
    void searchFinished(const DFMSEARCH::SearchResultList &results);

    /**
     * @brief Emitted when a search operation is cancelled
     */
    void searchCancelled();

    /**
     * @brief Emitted when an error occurs during search
     * @param error The SearchError that occurred
     */
    void errorOccurred(const DFMSEARCH::SearchError &error);

private:
    Q_DISABLE_COPY(SemanticSearcher)
    std::unique_ptr<SemanticSearcherData> d_ptr;
};

DFM_SEARCH_END_NS

#endif   // SEMANTICSEARCHER_H
