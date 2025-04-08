// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHOPTIONSDATA_H
#define SEARCHOPTIONSDATA_H

#include <QVariantHash>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The SearchOptionsData class provides the private implementation for SearchOptions
 * 
 * This class implements the PIMPL pattern for SearchOptions, containing all the
 * actual data members and implementation details.
 */
class SearchOptionsData
{
public:
    /**
     * @brief Default constructor
     */
    SearchOptionsData();

    /**
     * @brief Copy constructor
     * @param other The SearchOptionsData object to copy from
     */
    SearchOptionsData(const SearchOptionsData &other);

    // Public data fields
    SearchMethod method;           ///< The search method to use
    bool caseSensitive;           ///< Whether search is case sensitive
    QString searchPath;           ///< The path to search in
    bool includeHidden;           ///< Whether to include hidden files
    int maxResults;               ///< Maximum number of results to return
    QVariantHash customOptions;   ///< Custom search options
    bool enableResultFound;       ///< Whether to enable result found notifications
};

DFM_SEARCH_END_NS

#endif   // SEARCHOPTIONSDATA_H
