// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUCENEQUERYUTILS_H
#define LUCENEQUERYUTILS_H

#include <QString>
#include <QStringList>

#include <lucene++/LuceneHeaders.h>
#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The LuceneQueryUtils namespace provides utility functions for Lucene query operations
 */
namespace LuceneQueryUtils {

/**
 * @brief Process a string for Lucene query, escaping special characters
 * @param str The string to process
 * @param caseSensitive Whether the search is case sensitive
 * @return The processed Lucene::String
 */
Lucene::String processQueryString(const QString &str, bool caseSensitive = false);

/**
 * @brief Get a list of Lucene special characters that need escaping
 * @return List of special characters
 */
std::wstring getLuceneSpecialChars();

/**
 * @brief Build a path prefix query for Lucene
 * @param pathPrefix The path prefix to search for
 * @param fieldName The index field name (e.g., "full_path" or "path")
 * @return Lucene query object, or nullptr if pathPrefix is empty
 */
Lucene::QueryPtr buildPathPrefixQuery(const QString &pathPrefix, const QString &fieldName);

}   // namespace LuceneQueryUtils

DFM_SEARCH_END_NS

#endif   // LUCENEQUERYUTILS_H
