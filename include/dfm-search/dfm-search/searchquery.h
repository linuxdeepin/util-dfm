// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHQUERY_H
#define SEARCHQUERY_H

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class SearchQueryData;

/**
 * @brief The SearchQuery class encapsulates search query conditions
 * 
 * This class provides a flexible way to construct and manage search queries,
 * supporting both simple keyword searches and complex boolean queries.
 * It allows for combining multiple search terms using logical operators.
 */
class SearchQuery
{
public:
    /**
     * @brief Enumeration of query types
     */
    enum class Type {
        Simple,   ///< Simple keyword search
        Boolean,  ///< Boolean query (AND, OR)
        Wildcard  ///< Wildcard search with * and ? patterns
    };

    /**
     * @brief Enumeration of boolean operators
     */
    enum class BooleanOperator {
        AND,  ///< Logical AND operation
        OR    ///< Logical OR operation
        // TODO: Logical NOT operation
    };

    /**
     * @brief Default constructor
     */
    SearchQuery();

    /**
     * @brief Constructor with keyword
     * @param keyword The search keyword
     */
    explicit SearchQuery(const QString &keyword);

    /**
     * @brief Constructor with keyword and type
     * @param keyword The search keyword
     * @param type The query type
     */
    SearchQuery(const QString &keyword, Type type);

    /**
     * @brief Copy constructor
     * @param other The SearchQuery object to copy from
     */
    SearchQuery(const SearchQuery &other);

    /**
     * @brief Move constructor
     * @param other The SearchQuery object to move from
     */
    SearchQuery(SearchQuery &&other) noexcept;

    /**
     * @brief Destructor
     */
    ~SearchQuery();

    /**
     * @brief Copy assignment operator
     * @param other The SearchQuery object to copy from
     * @return Reference to this object
     */
    SearchQuery &operator=(const SearchQuery &other);

    /**
     * @brief Move assignment operator
     * @param other The SearchQuery object to move from
     * @return Reference to this object
     */
    SearchQuery &operator=(SearchQuery &&other) noexcept;

    /**
     * @brief Get the search keyword
     * @return The search keyword
     */
    QString keyword() const;

    /**
     * @brief Set the search keyword
     * @param keyword The new search keyword
     */
    void setKeyword(const QString &keyword);

    /**
     * @brief Get the query type
     * @return The current query type
     */
    Type type() const;

    /**
     * @brief Set the query type
     * @param type The new query type
     */
    void setType(Type type);

    /**
     * @brief Get the boolean operator
     * @return The current boolean operator
     */
    BooleanOperator booleanOperator() const;

    /**
     * @brief Set the boolean operator
     * @param op The new boolean operator
     */
    void setBooleanOperator(BooleanOperator op);

    /**
     * @brief Add a sub-query for boolean operations
     * @param query The sub-query to add
     */
    void addSubQuery(const SearchQuery &query);

    /**
     * @brief Get the list of sub-queries
     * @return List of sub-queries
     */
    QList<SearchQuery> subQueries() const;

    /**
     * @brief Clear all sub-queries
     */
    void clearSubQueries();

    /**
     * @brief Create a simple search query
     * @param keyword The search keyword
     * @return A new SearchQuery object
     */
    static SearchQuery createSimpleQuery(const QString &keyword);

    /**
     * @brief Create a boolean search query
     * @param keywords List of search keywords
     * @param op The boolean operator to use
     * @return A new SearchQuery object
     */
    static SearchQuery createBooleanQuery(const QStringList &keywords, BooleanOperator op = BooleanOperator::AND);

private:
    std::unique_ptr<SearchQueryData> d;
};

DFM_SEARCH_END_NS

Q_DECLARE_METATYPE(DFMSEARCH::SearchQuery);

#endif   // SEARCHQUERY_H
