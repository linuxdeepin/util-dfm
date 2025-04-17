// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHUTILITY_H
#define SEARCHUTILITY_H

#include <QStringList>
#include <QString>

// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchquery.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The SearchUtility namespace provides utility functions for search operations
 */
namespace SearchUtility {

/**
 * @brief Extract keywords from a boolean search query
 * @param query The search query to process
 * @return List of extracted keywords
 */
QStringList extractBooleanKeywords(const SearchQuery &query);

/**
 * @brief Get the list of supported file types for Deepin Anything
 * @return List of supported file types
 */
QStringList deepinAnythingFileTypes();

}   // namespace SearchUtility
DFM_SEARCH_END_NS

#endif   // SEARCHUTILITY_H
