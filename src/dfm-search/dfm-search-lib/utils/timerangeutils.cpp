// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "timerangeutils.h"

#include <dfm-search/timerangefilter.h>
#include <lucene++/NumericRangeQuery.h>

DFM_SEARCH_BEGIN_NS

namespace TimeRangeUtils {

qint64 toEpochSecs(const QDateTime &dt)
{
    if (!dt.isValid()) {
        return 0;
    }
    return dt.toSecsSinceEpoch();
}

Lucene::QueryPtr buildNumericRangeQuery(
    const wchar_t *fieldName,
    qint64 startEpoch,
    qint64 endEpoch,
    bool includeLower,
    bool includeUpper)
{
    // Use INT64_MIN and INT64_MAX for unbounded ranges
    int64_t minVal = (startEpoch == 0) ? INT64_MIN : startEpoch;
    int64_t maxVal = (endEpoch == 0) ? INT64_MAX : endEpoch;

    // Use the default precisionStep (4)
    return Lucene::NumericRangeQuery::newLongRange(
        Lucene::String(fieldName),
        minVal,
        maxVal,
        includeLower,
        includeUpper);
}

}   // namespace TimeRangeUtils

DFM_SEARCH_END_NS
