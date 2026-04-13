// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TIMERANGEUTILS_H
#define TIMERANGEUTILS_H

#include <QDateTime>

#include <lucene++/LuceneHeaders.h>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief TimeRangeUtils provides utility functions for time range operations
 */
namespace TimeRangeUtils {

/**
 * @brief Convert QDateTime to Unix timestamp in seconds
 * @param dt The datetime to convert
 * @return Unix timestamp in seconds (0 if invalid)
 */
qint64 toEpochSecs(const QDateTime &dt);

/**
 * @brief Build a Lucene NumericRangeQuery for time range filtering
 * @param fieldName The field name to query (e.g., L"birth_time", L"modify_time")
 * @param startEpoch The start timestamp (in seconds), use 0 for no lower bound
 * @param endEpoch The end timestamp (in seconds), use INT64_MAX for no upper bound
 * @param includeLower Whether to include the lower bound
 * @param includeUpper Whether to include the upper bound
 * @return A NumericRangeQuery pointer, or nullptr if range is invalid
 */
Lucene::QueryPtr buildNumericRangeQuery(
    const wchar_t *fieldName,
    qint64 startEpoch,
    qint64 endEpoch,
    bool includeLower,
    bool includeUpper);

}   // namespace TimeRangeUtils

DFM_SEARCH_END_NS

#endif   // TIMERANGEUTILS_H
