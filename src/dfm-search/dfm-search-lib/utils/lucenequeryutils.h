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
 * @brief Build a query that matches text indexed by NGramAnalyzer(1, 2)
 *
 * The query is built directly instead of passing user input through an n-gram
 * analyzer at search time. One- and two-character keywords use TermQuery.
 * Longer keywords use a sparse PhraseQuery over 2-grams to avoid generating
 * every overlapping query term.
 *
 * @param fieldName The indexed field name
 * @param keyword The raw user keyword
 * @param caseSensitive Whether the search is case sensitive
 * @return Lucene query object, or nullptr if fieldName or keyword is empty
 */
Lucene::QueryPtr buildNGramSearchQuery(const QString &fieldName, const QString &keyword, bool caseSensitive = false);

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

/**
 * @brief Build a multi-path prefix query for Lucene
 *
 * When multiple paths are provided, builds a BooleanQuery with SHOULD clauses
 * for each path. When only one path is provided, returns a simple pathPrefixQuery.
 *
 * @param paths List of path prefixes to search for
 * @param fieldName The index field name (e.g., "ancestor_paths")
 * @return Lucene query object, or nullptr if paths is empty
 */
Lucene::QueryPtr buildMultiPathPrefixQuery(const QStringList &paths, const QString &fieldName);

}   // namespace LuceneQueryUtils

DFM_SEARCH_END_NS

#endif   // LUCENEQUERYUTILS_H
