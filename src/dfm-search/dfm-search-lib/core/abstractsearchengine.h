// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ABSTRACTSEARCHENGINE_H
#define ABSTRACTSEARCHENGINE_H

#include <atomic>

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

#include <dfm-search/searchengine.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The AbstractSearchEngine class provides the base interface for all search engines
 *
 * This abstract class defines the common interface that all search engine implementations
 * must provide. It includes methods for search operations, status management, and error handling.
 */
class AbstractSearchEngine : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param parent The parent QObject
     */
    explicit AbstractSearchEngine(QObject *parent = nullptr);

    /**
     * @brief Virtual destructor
     */
    virtual ~AbstractSearchEngine();

    /**
     * @brief Initialize the search engine
     */
    virtual void init();

    /**
     * @brief Get the search type
     * @return The SearchType of this engine
     */
    virtual SearchType searchType() const = 0;

    /**
     * @brief Get the current search options
     * @return The current SearchOptions
     */
    virtual SearchOptions searchOptions() const = 0;

    /**
     * @brief Set the search options
     * @param options The new SearchOptions
     */
    virtual void setSearchOptions(const SearchOptions &options) = 0;

    /**
     * @brief Get the current search status
     * @return The current SearchStatus
     */
    virtual SearchStatus status() const = 0;

    /**
     * @brief Perform an asynchronous search
     * @param query The search query to execute
     */
    virtual void search(const SearchQuery &query) = 0;

    /**
     * @brief Perform an asynchronous search with callback
     * @param query The search query to execute
     * @param callback The callback function to process results
     */
    virtual void searchWithCallback(const SearchQuery &query,
                                    SearchEngine::ResultCallback callback) = 0;

    /**
     * @brief Perform a synchronous search
     * @param query The search query to execute
     * @return A SearchResultExpected containing the results or an error
     */
    virtual SearchResultExpected searchSync(const SearchQuery &query) = 0;

    /**
     * @brief Cancel the current search operation
     */
    virtual void cancel() = 0;

Q_SIGNALS:
    /**
     * @brief Emitted when a search operation starts
     */
    void searchStarted();

    /**
     * @brief Emitted when a new search result is found
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
     * @param results The list of all search results
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

protected:
    /**
     * @brief Set the current search status
     * @param status The new search status
     */
    void setStatus(SearchStatus status);

    /**
     * @brief Report a search error
     * @param error The SearchError to report
     */
    void reportError(const SearchError &error);

    std::atomic<SearchStatus> m_status;   ///< Current search status
    std::atomic<bool> m_cancelled;   ///< Search cancellation flag
};

DFM_SEARCH_END_NS

#endif   // ABSTRACTSEARCHENGINE_H
