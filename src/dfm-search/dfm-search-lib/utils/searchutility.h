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

/**
 * Determines if the given absolute path represents a hidden file/directory
 * or is contained within a hidden directory.
 *
 * A path is considered hidden if any component (except root) starts with '.'
 * and is not the special "." (current) or ".." (parent) directory.
 *
 * @param absolutePath The absolute filesystem path to check (must be normalized)
 * @return true if the path is hidden or inside hidden directories, false otherwise
 */
bool isHiddenPathOrInHiddenDir(const QString &absolutePath);
}   // namespace SearchUtility
DFM_SEARCH_END_NS

#endif   // SEARCHUTILITY_H
