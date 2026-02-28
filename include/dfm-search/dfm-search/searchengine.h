// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QObject>

#include <dfm-search/searchresult.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searcherror.h>

DFM_SEARCH_BEGIN_NS

class AbstractSearchEngine;

/**
 * @brief The SearchEngine class provides a unified interface for file searching
 *
 * This class serves as the main entry point for all search operations. It uses
 * the factory pattern to create specific search engine implementations based on
 * the search type. The class manages the search lifecycle and provides both
 * synchronous and asynchronous search capabilities.
 */
class SearchEngine : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Callback function type for search results
     * @param result The search result to process
     * @return bool If returns `true`, the search iteration will be interrupted
     */
    using ResultCallback = std::function<bool(const SearchResult &)>;

    /**
     * @brief Creates a search engine of the specified type
     * @param type The type of search engine to create
     * @param parent The parent QObject
     * @return A pointer to the created SearchEngine instance
     */
    static SearchEngine *create(SearchType type, QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SearchEngine() override;

    /**
     * @brief Get the current search type
     * @return The current SearchType
     */
    SearchType searchType() const;

    /**
     * @brief Set the search type
     * @param type The new SearchType
     */
    void setSearchType(SearchType type);

    /**
     * @brief Get the current search options
     * @return The current SearchOptions
     */
    SearchOptions searchOptions() const;

    /**
     * @brief Set the search options
     * @param options The new SearchOptions
     */
    void setSearchOptions(const SearchOptions &options);

    /**
     * @brief Get the current search status
     * @return The current SearchStatus
     */
    SearchStatus status() const;

    /**
     * @brief Perform an asynchronous search
     * @param query The search query to execute
     */
    void search(const SearchQuery &query);

    /**
     * @brief Perform an asynchronous search with a callback
     * @param query The search query to execute
     * @param callback The callback function to process results
     */
    void searchWithCallback(const SearchQuery &query, ResultCallback callback);

    /**
     * @brief Perform a synchronous search
     * @param query The search query to execute
     * @return A SearchResultExpected containing the results or an error
     */
    SearchResultExpected searchSync(const SearchQuery &query);

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
    explicit SearchEngine(QObject *parent = nullptr);
    SearchEngine(SearchType type, QObject *parent = nullptr);

    friend class SearchFactory;

private:
    std::unique_ptr<AbstractSearchEngine> d_ptr;
};

DFM_SEARCH_END_NS

#endif   // SEARCHENGINE_H
