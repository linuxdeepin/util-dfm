// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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
    void enableResultFound(bool enable);

    /**
     * @brief Returns whether per-file result notifications are enabled.
     *
     * @return @c true if detailed per-file results are being emitted (with performance
     * overhead), @c false otherwise.
     */
    bool resultFoundEnabled() const;

private:
    std::unique_ptr<SearchOptionsData> d;   // PIMPL
};

DFM_SEARCH_END_NS

#endif   // SEARCHOPTIONS_H
