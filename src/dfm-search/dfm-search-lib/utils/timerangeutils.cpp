// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "timerangeutils.h"

#include <dfm-search/timerangefilter.h>
#include <lucene++/NumericRangeQuery.h>
#include <lucene++/BooleanQuery.h>

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

Lucene::QueryPtr buildTimeRangeFilterQuery(
        const TimeRangeFilter &filter,
        const wchar_t *birthTimeField,
        const wchar_t *modifyTimeField)
{
    if (!filter.isValid()) {
        return nullptr;
    }

    auto [start, end] = filter.resolveTimeRange();
    qint64 startEpoch = toEpochSecs(start);
    qint64 endEpoch = toEpochSecs(end);

    if (filter.timeField() == TimeField::Both) {
        // Build BooleanQuery with SHOULD for both time fields
        Lucene::BooleanQueryPtr timeBoolQuery = Lucene::newLucene<Lucene::BooleanQuery>();

        Lucene::QueryPtr birthQuery = buildNumericRangeQuery(
                birthTimeField, startEpoch, endEpoch,
                filter.includeLower(), filter.includeUpper());
        if (birthQuery) {
            timeBoolQuery->add(birthQuery, Lucene::BooleanClause::SHOULD);
        }

        Lucene::QueryPtr modifyQuery = buildNumericRangeQuery(
                modifyTimeField, startEpoch, endEpoch,
                filter.includeLower(), filter.includeUpper());
        if (modifyQuery) {
            timeBoolQuery->add(modifyQuery, Lucene::BooleanClause::SHOULD);
        }

        // Need at least one clause for a valid BooleanQuery
        return (birthQuery || modifyQuery) ? timeBoolQuery : nullptr;
    }

    // Single field query
    const wchar_t *fieldName = (filter.timeField() == TimeField::BirthTime)
            ? birthTimeField
            : modifyTimeField;

    return buildNumericRangeQuery(
            fieldName, startEpoch, endEpoch,
            filter.includeLower(), filter.includeUpper());
}

}   // namespace TimeRangeUtils

DFM_SEARCH_END_NS
