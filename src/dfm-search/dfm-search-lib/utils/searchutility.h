// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHUTILITY_H
#define SEARCHUTILITY_H

#include <QStringList>
#include <QString>
#include <QSet>
#include <QVariant>
#include <QByteArray>
#include <optional>

// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchquery.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief The SearchUtility namespace provides utility functions for search operations
 */
namespace Global {

/**
 * @brief Map filename index status JSON content (index_status.json) to the legacy status string
 *
 * Mapping (keeps legacy semantics):
 * - std::nullopt : JSON missing/invalid
 * - "scanning"   : createInProgress or updateInProgress is set (full build / recovery /
 *                  rebuild update in progress, index not trustworthy), or no successful
 *                  task has ever completed (empty lastUpdateTime)
 * - "updating"   : backlogExceeded is set (large event backlog in progress, index
 *                  severely lagged — replicates legacy "updating" semantics)
 * - "monitoring" : lastUpdateTime present and no in-progress/backlog flags (index
 *                  searchable, ordinary incremental updates may still be running)
 */
std::optional<QString> fileNameIndexStatusFromJson(const QByteArray &jsonContent);

/**
 * @brief Check whether index_status.json content marks the filename index ready for search
 *
 * Five conditions: valid JSON && lastUpdateTime non-empty && !createInProgress
 * && !updateInProgress && !backlogExceeded. The dirty/clean state is not checked:
 * ordinary incremental updates keep the index searchable, while recovery/rebuild
 * updates and large event backlogs do set their respective flags and must degrade
 * search to realtime traversal.
 */
bool fileNameIndexReadyForSearchFromJson(const QByteArray &jsonContent);

}   // namespace Global

namespace SearchUtility {

/**
 * @brief Extract keywords from a boolean search query
 * @param query The search query to process
 * @return List of extracted keywords
 */
QStringList extractBooleanKeywords(const SearchQuery &query);

/**
 * @brief Parse a DConfig QVariant value into a QStringList
 *
 * DConfig values may arrive as a semicolon-separated string or as a JSON array
 * (QVariant::List / QVariant::StringList). When the value is a list type,
 * toString() returns an empty string, so toStringList() must be used instead.
 *
 * @param value The QVariant from DConfig::value()
 * @return The parsed string list, or std::nullopt if the value is invalid or not convertible
 */
std::optional<QStringList> parseVariantToStringList(const QVariant &value);

/**
 * @brief Get the list of supported file types for Deepin Anything
 * @return List of supported file types
 */
QStringList deepinAnythingFileTypes();

/**
 * @brief Get document file extensions for semantic search routing
 *
 * Loads from dconfig `org.deepin.anything` key `doc_file_suffix`.
 * @return Set of lowercase extensions (without leading dot), empty if dconfig unavailable
 */
QSet<QString> semanticDocExtensions();

/**
 * @brief Get image file extensions for semantic search routing
 *
 * Loads from dconfig `org.deepin.anything` key `pic_file_suffix`.
 * @return Set of lowercase extensions (without leading dot), empty if dconfig unavailable
 */
QSet<QString> semanticPicExtensions();

}   // namespace SearchUtility

DFM_SEARCH_END_NS
#endif   // SEARCHUTILITY_H
