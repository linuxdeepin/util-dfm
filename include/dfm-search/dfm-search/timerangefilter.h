// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TIMERANGEFILTER_H
#define TIMERANGEFILTER_H

#include <QDateTime>
#include <QPair>
#include <memory>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class TimeRangeFilterData;

/**
 * @brief The TimeRangeFilter class provides time range filtering for search operations.
 *
 * This class provides a fluent interface for specifying time ranges:
 *
 * Example usage:
 * @code
 * // Files modified in last 3 days
 * TimeRangeFilter filter;
 * filter.setTimeField(TimeField::ModifyTime).setLast(3, TimeUnit::Days);
 *
 * // Files created today
 * TimeRangeFilter filter;
 * filter.setTimeField(TimeField::BirthTime).setToday();
 *
 * // Custom time range
 * TimeRangeFilter filter;
 * filter.setTimeField(TimeField::ModifyTime).setRange(startDate, endDate);
 * @endcode
 */
class TimeRangeFilter
{
public:
    /**
     * @brief Default constructor
     * Creates an invalid filter (no time range set)
     */
    TimeRangeFilter();

    /**
     * @brief Copy constructor
     */
    TimeRangeFilter(const TimeRangeFilter &other);

    /**
     * @brief Move constructor
     */
    TimeRangeFilter(TimeRangeFilter &&other) noexcept;

    /**
     * @brief Destructor
     */
    ~TimeRangeFilter();

    /**
     * @brief Assignment operator
     */
    TimeRangeFilter &operator=(const TimeRangeFilter &other);

    /**
     * @brief Move assignment operator
     */
    TimeRangeFilter &operator=(TimeRangeFilter &&other) noexcept;

    // ---------- Time Field ----------

    /**
     * @brief Set the time field to filter on
     * @param field The time field (BirthTime or ModifyTime)
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setTimeField(TimeField field);

    /**
     * @brief Get the time field being filtered on
     * @return The current time field
     */
    TimeField timeField() const;

    // ---------- Relative Time Range (Fluent Interface) ----------

    /**
     * @brief Set a relative time range from now
     * @param value The number of time units
     * @param unit The time unit (Minutes, Hours, Days, Weeks, Months, Years)
     * @return Reference to this filter for method chaining
     *
     * Example: setLast(3, TimeUnit::Days) means files from 3 days ago to now
     */
    TimeRangeFilter &setLast(int value, TimeUnit unit);

    // ---------- Fixed Presets ----------

    /**
     * @brief Set range to today (from 00:00:00 today to 00:00:00 tomorrow)
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setToday();

    /**
     * @brief Set range to yesterday
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setYesterday();

    /**
     * @brief Set range to this week (from Monday 00:00:00 to next Monday 00:00:00)
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setThisWeek();

    /**
     * @brief Set range to last week
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setLastWeek();

    /**
     * @brief Set range to this month (from 1st day 00:00:00 to 1st day of next month)
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setThisMonth();

    /**
     * @brief Set range to last month
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setLastMonth();

    /**
     * @brief Set range to this year (from Jan 1st to Jan 1st of next year)
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setThisYear();

    /**
     * @brief Set range to last year
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setLastYear();

    // ---------- Custom Time Range ----------

    /**
     * @brief Set a custom time range
     * @param start The start datetime of the range
     * @param end The end datetime of the range
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setRange(const QDateTime &start, const QDateTime &end);

    /**
     * @brief Get the custom start time
     * @return The start datetime
     */
    QDateTime startTime() const;

    /**
     * @brief Get the custom end time
     * @return The end datetime
     */
    QDateTime endTime() const;

    // ---------- Boundary Control ----------

    /**
     * @brief Set whether the lower bound is inclusive
     * @param include true to include the lower bound
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setIncludeLower(bool include);

    /**
     * @brief Set whether the upper bound is inclusive
     * @param include true to include the upper bound
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &setIncludeUpper(bool include);

    /**
     * @brief Check if lower bound is inclusive
     * @return true if lower bound is inclusive
     */
    bool includeLower() const;

    /**
     * @brief Check if upper bound is inclusive
     * @return true if upper bound is inclusive
     */
    bool includeUpper() const;

    // ---------- Filter State ----------

    /**
     * @brief Clear the filter (make it invalid)
     * @return Reference to this filter for method chaining
     */
    TimeRangeFilter &clear();

    /**
     * @brief Check if the filter is valid
     * @return true if a time range is set
     */
    bool isValid() const;

    /**
     * @brief Resolve the actual time range
     * Calculates the actual start/end times based on current time for relative ranges.
     * @return A pair of (start, end) datetimes
     */
    QPair<QDateTime, QDateTime> resolveTimeRange() const;

    /**
     * @brief Resolve a relative time range to actual datetime range
     * @param value The number of time units
     * @param unit The time unit
     * @return A pair of (start, end) datetimes
     */
    static QPair<QDateTime, QDateTime> resolveRelativeTimeRange(int value, TimeUnit unit);

private:
    /**
     * @brief Resolve a fixed unit time range to actual datetime range
     * @param value The number of time units (0 = this unit, 1 = last unit, etc.)
     * @param unit The time unit
     * @return A pair of (start, end) datetimes
     */
    static QPair<QDateTime, QDateTime> resolveFixedUnitTimeRange(int value, TimeUnit unit);

    std::unique_ptr<TimeRangeFilterData> d;
};

DFM_SEARCH_END_NS

#endif   // TIMERANGEFILTER_H
