// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHUTILITY_H
#define SEARCHUTILITY_H

#include <QStringList>
#include <QString>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchquery.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The SearchUtility namespace provides utility functions for search operations
 */
namespace SearchUtility {

/**
 * @brief Get the content index directory path
 * @return The path to the content index directory
 */
QString contentIndexDirectory();

/**
 * @brief Get the system index directory path
 * @return The path to the system index directory
 */
QString anythingIndexDirectory();

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
 * @brief Check if a string contains only pinyin characters
 * @param str The string to check
 * @return true if the string contains only pinyin, false otherwise
 */
bool isPurePinyin(const QString &str);

}   // namespace SearchUtility
DFM_SEARCH_END_NS

#endif   // SEARCHUTILITY_H
