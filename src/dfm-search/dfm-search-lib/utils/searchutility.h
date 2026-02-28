// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHUTILITY_H
#define SEARCHUTILITY_H

#include <QStringList>
#include <QString>

// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
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

/**
 * @brief Check if path prefix query optimization should be used
 * @param searchPath The search path
 * @return true if path prefix query should be used, false otherwise
 */
bool shouldUsePathPrefixQuery(const QString &searchPath);

/**
 * @brief Check if the filename index supports the ancestor_paths field.
 * This function checks the filename index version and returns true if the version is greater than 3.
 * @return true if the filename index supports ancestor_paths, false otherwise.
 */
bool isFilenameIndexAncestorPathsSupported();

/**
 * @brief Check if the content index supports the ancestor_paths field.
 * This function checks the content index version and returns true if the version is greater than 1.
 * @return true if the content index supports ancestor_paths, false otherwise.
 */
bool isContentIndexAncestorPathsSupported();

}   // namespace SearchUtility
DFM_SEARCH_END_NS

#endif   // SEARCHUTILITY_H
