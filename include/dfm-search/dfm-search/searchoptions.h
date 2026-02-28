// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHOPTIONS_H
#define SEARCHOPTIONS_H

#include <QString>
#include <QStringList>
#include <QVariant>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class SearchOptionsData;

class SearchOptions
{
public:
    /**
     * @brief Constructor
     */
    SearchOptions();

    /**
     * @brief Copy constructor
     */
    SearchOptions(const SearchOptions &other);

    /**
     * @brief Move constructor
     */
    SearchOptions(SearchOptions &&other) noexcept;

    /**
     * @brief Destructor
     */
    virtual ~SearchOptions();

    /**
     * @brief Assignment operator
     */
    SearchOptions &operator=(const SearchOptions &other);

    /**
     * @brief Move assignment operator
     */
    SearchOptions &operator=(SearchOptions &&other) noexcept;

    /**
     * @brief Get the search method
     */
    SearchMethod method() const;

    /**
     * @brief Set the search method
     * Indexed search is suitable for directories with established indexes,
     * it is fast but may not provide the latest results.
     * Real-time search directly scans the file system,
     * the results are the latest but may be slower.
     */
    void setSearchMethod(SearchMethod method);

    /**
     * @brief Check if case sensitivity is enabled
     */
    bool caseSensitive() const;

    /**
     * @brief Set case sensitivity
     */
    void setCaseSensitive(bool sensitive);

    /**
     * @brief Get the starting search path
     */
    QString searchPath() const;

    /**
     * @brief Set the starting search path
     */
    void setSearchPath(const QString &path);

    /**
     * @brief Returns the current list of excluded search paths.
     *
     * @return A list of absolute directory paths that are excluded from searches.
     *         Returns an empty list if no exclusions are set.
     */
    QStringList searchExcludedPaths() const;

    /**
     * @brief Sets the list of excluded paths for search operations.
     *
     * When a path is in this list, it and its contents will be skipped during searches.
     * Paths should be absolute and normalized (use QDir::cleanPath for consistency).
     *
     * @param excludedPaths A list of directory paths to exclude from searches.
     *                      Empty list means no exclusions.
     */
    void setSearchExcludedPaths(const QStringList &excludedPaths);

    /**
     * @brief Set whether to include hidden files
     */
    void setIncludeHidden(bool include);

    /**
     * @brief Check if hidden files are included
     */
    bool includeHidden() const;

    /**
     * @brief Get the maximum result count limit
     */
    int maxResults() const;

    /**
     * @brief Set the maximum result count limit
     */
    void setMaxResults(int count);

    /**
     * @brief Set a custom option
     */
    void setCustomOption(const QString &key, const QVariant &value);

    /**
     * @brief Get a custom option
     */
    QVariant customOption(const QString &key) const;

    /**
     * @brief Check if a specific custom option is set
     */
    bool hasCustomOption(const QString &key) const;

    /**
     * @brief Enables or disables detailed per-file search result notifications.
     *
     * When enabled, the signal @c resultFound will be emitted for each matching file,
     * providing more detailed search results. Note that this may cause significant
     * performance overhead, especially in index-based search modes where it is
     * NOT RECOMMENDED.
     *
     * @param enable Set @c true to enable per-file notifications, @c false to disable.
     */
    void setResultFoundEnabled(bool enable);

    /**
     * @brief Returns whether per-file result notifications are enabled.
     *
     * @return @c true if detailed per-file results are being emitted (with performance
     * overhead), @c false otherwise.
     */
    bool resultFoundEnabled() const;

    /**
     * @brief Enables or disables the addition of detailed information to search results.
     *
     * When enabled, search results will include additional metadata such as file size,
     * modification time, file type, etc. When disabled, only the basic path information
     * is included, which improves performance and reduces memory usage.
     *
     * This is separate from resultFoundEnabled() which controls whether result signals
     * are emitted during the search process.
     *
     * @param enable Set @c true to include detailed information in results, @c false for basic results only.
     */
    void setDetailedResultsEnabled(bool enable);

    /**
     * @brief Returns whether search results include detailed information.
     *
     * @return @c true if search results include detailed metadata, @c false if results
     * only contain basic path information.
     */
    bool detailedResultsEnabled() const;

    /**
     * @brief Sets the synchronization search timeout period in seconds.
     *
     * This timeout controls how long the system waits for search operations to
     * complete before aborting. Set to 0 to disable timeout (not recommended
     * for production environments).
     *
     * @param seconds Timeout duration in seconds (minimum 1, maximum 300)
     * @sa syncSearchTimeout()
     */
    void setSyncSearchTimeout(int seconds);

    /**
     * @brief Returns the current synchronization search timeout in seconds.
     *
     * @return Timeout duration in seconds (defualt is 60)
     * @sa setSyncSearchTimeout()
     */
    int syncSearchTimeout() const;

    /**
     * @brief Sets the batch processing time interval in milliseconds.
     *
     * This controls how frequently search results are batched and emitted during
     * asynchronous search operations. A larger interval reduces the frequency of
     * signal emissions but may make the UI feel less responsive. A smaller interval
     * provides more responsive updates but may impact performance with high result volumes.
     *
     * @param milliseconds Batch time interval in milliseconds (minimum 50, maximum 5000)
     * @sa batchTime()
     */
    void setBatchTime(int milliseconds);

    /**
     * @brief Returns the current batch processing time interval in milliseconds.
     *
     * @return Batch time interval in milliseconds (default is 1000)
     * @sa setBatchTime()
     */
    int batchTime() const;

private:
    std::unique_ptr<SearchOptionsData> d;   // PIMPL
};

DFM_SEARCH_END_NS

Q_DECLARE_METATYPE(DFMSEARCH::SearchOptions);

#endif   // SEARCHOPTIONS_H
