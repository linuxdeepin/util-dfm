// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTICSEARCHER_H
#define SEMANTICSEARCHER_H

#include <QObject>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searcherror.h>
#include <dfm-search/searchresult.h>
#include <dfm-search/semantic_types.h>

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
     * @brief Perform a semantic search with explicit search directories
     *
     * When @p searchDirectories is non-empty, those directories take priority
     * over any directories resolved from the natural language input.
     * If empty, falls back to NLP-parsed directories, then home directory.
     *
     * @param naturalLanguage The natural language query string
     * @param searchDirectories Explicit directories to search in
     */
    void search(const QString &naturalLanguage, const QStringList &searchDirectories);

    /**
     * @brief Parse natural language input into a structured intent.
     *
     * Synchronously runs the full dimension extractor pipeline (time, size,
     * file type, action, location, keyword) and returns the result. Does NOT
     * start any search. Use this when you need to inspect what the NLP parser
     * understood without triggering a search — e.g., for intent preview UIs,
     * debugging, or composing custom search plans.
     *
     * Equivalent to the parsing performed internally by search() before
     * searchStarted fires; the same ParsedIntent is emitted via intentParsed().
     *
     * @param input The natural language query string
     * @return The parsed intent. If parsing yields no constraints, the returned
     *         ParsedIntent has an invalid timeConstraint, invalid sizeConstraint,
     *         empty fileExtensions, empty searchDirectories, and empty consumedSpans.
     *
     * @see isSemanticQuery() for a boolean check built on top of this method.
     */
    ParsedIntent parseIntent(const QString &input) const;

    /**
     * @brief Check if the input contains semantic intent beyond a plain keyword.
     *
     * Returns true if parsing the input reveals time constraints, size constraints,
     * file type filters, or location constraints. Returns false for plain keyword input.
     *
     * This is a convenience wrapper around parseIntent(): it parses once and
     * checks whether any semantic dimension was extracted. The parsing result
     * is discarded; if you need the parsed intent, call parseIntent() directly
     * to avoid parsing twice.
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
     * @brief Perform a synchronous semantic search with explicit directories
     * @param naturalLanguage The natural language query string
     * @param searchDirectories Explicit directories to search in
     * @return SearchResultExpected containing deduplicated results or an error
     */
    SearchResultExpected searchSync(const QString &naturalLanguage, const QStringList &searchDirectories);

    /**
     * @brief Cancel the current search operation
     */
    void cancel();

    /**
     * @brief Enable or disable detailed results for sub-engines
     *
     * When enabled, each sub-engine (FileName, Content, OCR) will populate
     * extra metadata fields (file type, size, timestamps, etc.) in results.
     * Must be called before search().
     *
     * @param enable true to enable detailed results (default false)
     */
    void setDetailedResultsEnabled(bool enable);

    /**
     * @brief Check whether detailed results are enabled
     */
    bool isDetailedResultsEnabled() const;

    /**
     * @brief Set the maximum number of results to return
     *
     * Each sub-engine (FileName, Content, OCR) will be limited to this count.
     * After all engines finish, results are deduplicated and truncated
     * to this count.
     *
     * @param count Maximum result count (0 = unlimited, default 0)
     */
    void setMaxResults(int count);

    /**
     * @brief Get the maximum number of results
     * @return Maximum result count (0 = unlimited)
     */
    int maxResults() const;

Q_SIGNALS:
    /**
     * @brief Emitted after the natural language input is parsed into an intent
     *
     * This fires before searchStarted(), allowing callers to inspect
     * what the NLP parser understood from the input.
     *
     * @param intent The parsed intent structure
     */
    void intentParsed(const DFMSEARCH::ParsedIntent &intent);

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
