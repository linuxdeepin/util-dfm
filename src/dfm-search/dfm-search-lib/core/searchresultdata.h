// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCH_RESULT_DATA_H
#define SEARCH_RESULT_DATA_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The SearchResultData class provides the base private implementation for SearchResult
 *
 * This class implements the PIMPL pattern for SearchResult, containing only the
 * data fields that are common to all types of search results.
 */
class SearchResultData
{
public:
    /**
     * @brief Default constructor
     */
    SearchResultData();

    /**
     * @brief Constructor with path
     * @param path The file path
     */
    SearchResultData(const QString &path);

    /**
     * @brief Copy constructor
     * @param other The SearchResultData object to copy from
     */
    SearchResultData(const SearchResultData &other);

    /**
     * @brief Default destructor
     */
    ~SearchResultData() = default;

    /**
     * @brief Move constructor
     * @param other The SearchResultData object to move from
     */
    SearchResultData(SearchResultData &&other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other The SearchResultData object to move from
     * @return Reference to this object
     */
    SearchResultData &operator=(SearchResultData &&other) noexcept;

    // Public data fields
    QString path;   ///< The file path
    QVariantMap customAttributes;   ///< Custom attributes associated with the result
};

DFM_SEARCH_END_NS

#endif   // SEARCH_RESULT_DATA_H
