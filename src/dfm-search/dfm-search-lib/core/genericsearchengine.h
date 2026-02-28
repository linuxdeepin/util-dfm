// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GENERICSEARCHENGINE_H
#define GENERICSEARCHENGINE_H

#include <QThread>
#include <QMutex>
#include <QTimer>

#include "abstractsearchengine.h"
#include "searchstrategy/searchworker.h"

DFM_SEARCH_BEGIN_NS

/**
 * @brief The GenericSearchEngine class provides a base implementation for all search engines
 *
 * This class implements the common functionality required by all search engines,
 * including thread management, result handling, and error reporting. It serves as
 * a base class for specific search engine implementations.
 */
class GenericSearchEngine : public AbstractSearchEngine
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent The parent QObject
     */
    explicit GenericSearchEngine(QObject *parent = nullptr);

    /**
     * @brief Virtual destructor
     */
    virtual ~GenericSearchEngine();

    /**
     * @brief Initialize the search engine and its worker thread
     */
    virtual void init() override;

    /**
     * @brief Get the current search options
     * @return The current SearchOptions
     */
    SearchOptions searchOptions() const override;

    /**
     * @brief Set the search options
     * @param options The new SearchOptions
     */
    void setSearchOptions(const SearchOptions &options) override;

    /**
     * @brief Get the current search status
     * @return The current SearchStatus
     */
    SearchStatus status() const override;

    /**
     * @brief Perform an asynchronous search
     * @param query The search query to execute
     */
    void search(const SearchQuery &query) override;

    /**
     * @brief Perform an asynchronous search with callback
     * @param query The search query to execute
     * @param callback The callback function to process results
     */
    void searchWithCallback(const SearchQuery &query,
                            SearchEngine::ResultCallback callback) override;

    /**
     * @brief Perform a synchronous search
     * @param query The search query to execute
     * @return A SearchResultExpected containing the results or an error
     */
    SearchResultExpected searchSync(const SearchQuery &query) override;

    /**
     * @brief Cancel the current search operation
     */
    void cancel() override;

Q_SIGNALS:
    /**
     * @brief Internal signal to request worker thread to execute search
     */
    void requestSearch(const DFMSEARCH::SearchQuery &query,
                      const DFMSEARCH::SearchOptions &options,
                      DFMSEARCH::SearchType searchType);

    /**
     * @brief Internal signal to request worker thread to cancel search
     */
    void requestCancel();

protected:
    /**
     * @brief Set up the strategy factory for this search engine
     *
     * This pure virtual method must be implemented by derived classes
     * to provide the appropriate search strategy factory.
     */
    virtual void setupStrategyFactory() = 0;

    /**
     * @brief Execute a synchronous search operation
     * @param query The search query to execute
     * @return A SearchResultExpected containing the results or an error
     */
    SearchResultExpected doSyncSearch(const SearchQuery &query);

    /**
     * @brief Validate search conditions
     * @return A SearchError indicating any validation errors
     */
    virtual SearchError validateSearchConditions();

private Q_SLOTS:
    /**
     * @brief Handle a new search result
     * @param result The found search result
     */
    void handleSearchResult(const DFMSEARCH::SearchResult &result);

    /**
     * @brief Handle search completion
     * @param results The list of all search results
     */
    void handleSearchFinished(const DFMSEARCH::SearchResultList &results);

    /**
     * @brief Handle search errors
     * @param error The SearchError that occurred
     */
    void handleErrorOccurred(const DFMSEARCH::SearchError &error);

protected:
    SearchOptions m_options;   ///< Current search options
    SearchQuery m_currentQuery;   ///< Current search query
    SearchEngine::ResultCallback m_callback;   ///< Current result callback
    SearchResultList m_results;   ///< List of search results

    QThread m_workerThread;   ///< Worker thread for search operations
    SearchWorker *m_worker;   ///< Search worker object
    SearchError m_lastError;   ///< Last occurred error

    // Result batching members
    QTimer m_batchTimer;   ///< Timer for batch sending results
    SearchResultList m_batchResults;   ///< Cached results waiting to be sent
};

DFM_SEARCH_END_NS

#endif   // GENERICSEARCHENGINE_H
