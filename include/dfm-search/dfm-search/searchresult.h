// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <DExpected>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searcherror.h>

DFM_SEARCH_BEGIN_NS

class SearchResultData;

/**
 * @brief The SearchResult class represents a single search result item.
 * 
 * This class encapsulates all information about a search result, including its path
 * and any custom attributes that may be associated with it. It provides methods to
 * access and modify these properties.
 */
class SearchResult
{
public:
    /**
     * @brief Default constructor
     * Creates an empty search result object
     */
    SearchResult();

    /**
     * @brief Constructor with path
     * @param path The file path of the search result
     */
    explicit SearchResult(const QString &path);

    /**
     * @brief Copy constructor
     * @param other The SearchResult object to copy from
     */
    SearchResult(const SearchResult &other);

    /**
     * @brief Move constructor
     * @param other The SearchResult object to move from
     */
    SearchResult(SearchResult &&other) noexcept;

    /**
     * @brief Virtual destructor
     */
    virtual ~SearchResult();

    /**
     * @brief Copy assignment operator
     * @param other The SearchResult object to copy from
     * @return Reference to this object
     */
    SearchResult &operator=(const SearchResult &other);

    /**
     * @brief Move assignment operator
     * @param other The SearchResult object to move from
     * @return Reference to this object
     */
    SearchResult &operator=(SearchResult &&other) noexcept;

    /**
     * @brief Get the file path of the search result
     * @return The file path as a QString
     */
    QString path() const;

    /**
     * @brief Set the file path of the search result
     * @param path The new file path
     */
    void setPath(const QString &path);

    /**
     * @brief Set a custom attribute for the search result
     * @param key The attribute key
     * @param value The attribute value
     */
    void setCustomAttribute(const QString &key, const QVariant &value);

    /**
     * @brief Get a custom attribute value
     * @param key The attribute key
     * @return The attribute value as QVariant
     */
    QVariant customAttribute(const QString &key) const;

    /**
     * @brief Check if a custom attribute exists
     * @param key The attribute key to check
     * @return true if the attribute exists, false otherwise
     */
    bool hasCustomAttribute(const QString &key) const;

    /**
     * @brief Get all custom attributes
     * @return A map of all custom attributes
     */
    QVariantMap customAttributes() const;

protected:
    std::unique_ptr<SearchResultData> d;
};

using SearchResultList = QList<DFMSEARCH::SearchResult>;
using SearchResultExpected = Dtk::Core::DExpected<QList<SearchResult>, DFMSEARCH::SearchError>;

DFM_SEARCH_END_NS

Q_DECLARE_METATYPE(DFMSEARCH::SearchResult)
Q_DECLARE_METATYPE(DFMSEARCH::SearchResultList)

#endif   // SEARCHRESULT_H
