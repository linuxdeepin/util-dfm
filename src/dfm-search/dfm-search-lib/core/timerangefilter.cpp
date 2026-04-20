// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/timerangefilter.h>

#include <QDate>

DFM_SEARCH_BEGIN_NS

/**
 * @brief Internal enum for range mode
 */
enum class RangeMode {
    Invalid,      // No range set
    Relative,     // Relative time (setLast) - rolling range from N units ago to now
    FixedUnit,    // Fixed unit range (yesterday, last week, etc.) - complete unit
    Custom        // Custom start/end
};

class TimeRangeFilterData
{
public:
    TimeRangeFilterData()
        : field(TimeField::ModifyTime)
        , mode(RangeMode::Invalid)
        , relativeValue(0)
        , relativeUnit(TimeUnit::Days)
        , includeLower(true)
        , includeUpper(false)
    {
    }

    TimeRangeFilterData(const TimeRangeFilterData &other)
        : field(other.field)
        , mode(other.mode)
        , relativeValue(other.relativeValue)
        , relativeUnit(other.relativeUnit)
        , startTime(other.startTime)
        , endTime(other.endTime)
        , includeLower(other.includeLower)
        , includeUpper(other.includeUpper)
    {
    }

    TimeField field;
    RangeMode mode;

    // For relative/fixed mode
    int relativeValue;
    TimeUnit relativeUnit;

    // For custom mode
    QDateTime startTime;
    QDateTime endTime;

    bool includeLower;
    bool includeUpper;
};

TimeRangeFilter::TimeRangeFilter()
    : d(std::make_unique<TimeRangeFilterData>())
{
}

TimeRangeFilter::TimeRangeFilter(const TimeRangeFilter &other)
    : d(std::make_unique<TimeRangeFilterData>(*other.d))
{
}

TimeRangeFilter::TimeRangeFilter(TimeRangeFilter &&other) noexcept
    : d(std::move(other.d))
{
}

TimeRangeFilter::~TimeRangeFilter() = default;

TimeRangeFilter &TimeRangeFilter::operator=(const TimeRangeFilter &other)
{
    if (this != &other) {
        d = std::make_unique<TimeRangeFilterData>(*other.d);
    }
    return *this;
}

TimeRangeFilter &TimeRangeFilter::operator=(TimeRangeFilter &&other) noexcept
{
    if (this != &other) {
        d = std::move(other.d);
    }
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setTimeField(TimeField field)
{
    d->field = field;
    return *this;
}

TimeField TimeRangeFilter::timeField() const
{
    return d->field;
}

TimeRangeFilter &TimeRangeFilter::setLast(int value, TimeUnit unit)
{
    d->mode = RangeMode::Relative;  // Rolling range from N units ago to now
    d->relativeValue = value;
    d->relativeUnit = unit;
    d->startTime = QDateTime();
    d->endTime = QDateTime();
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setToday()
{
    d->mode = RangeMode::FixedUnit;  // Complete unit (today)
    d->relativeValue = 0;
    d->relativeUnit = TimeUnit::Days;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setYesterday()
{
    d->mode = RangeMode::FixedUnit;  // Complete unit (yesterday)
    d->relativeValue = 1;
    d->relativeUnit = TimeUnit::Days;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setThisWeek()
{
    d->mode = RangeMode::FixedUnit;
    d->relativeValue = 0;
    d->relativeUnit = TimeUnit::Weeks;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setLastWeek()
{
    d->mode = RangeMode::FixedUnit;
    d->relativeValue = 1;
    d->relativeUnit = TimeUnit::Weeks;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setThisMonth()
{
    d->mode = RangeMode::FixedUnit;
    d->relativeValue = 0;
    d->relativeUnit = TimeUnit::Months;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setLastMonth()
{
    d->mode = RangeMode::FixedUnit;
    d->relativeValue = 1;
    d->relativeUnit = TimeUnit::Months;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setThisYear()
{
    d->mode = RangeMode::FixedUnit;
    d->relativeValue = 0;
    d->relativeUnit = TimeUnit::Years;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setLastYear()
{
    d->mode = RangeMode::FixedUnit;
    d->relativeValue = 1;
    d->relativeUnit = TimeUnit::Years;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setRange(const QDateTime &start, const QDateTime &end)
{
    d->mode = RangeMode::Custom;
    d->startTime = start;
    d->endTime = end;
    d->relativeValue = 0;
    d->relativeUnit = TimeUnit::Days;
    return *this;
}

QDateTime TimeRangeFilter::startTime() const
{
    return d->startTime;
}

QDateTime TimeRangeFilter::endTime() const
{
    return d->endTime;
}

TimeRangeFilter &TimeRangeFilter::setIncludeLower(bool include)
{
    d->includeLower = include;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setIncludeUpper(bool include)
{
    d->includeUpper = include;
    return *this;
}

bool TimeRangeFilter::includeLower() const
{
    return d->includeLower;
}

bool TimeRangeFilter::includeUpper() const
{
    return d->includeUpper;
}

TimeRangeFilter &TimeRangeFilter::clear()
{
    d->mode = RangeMode::Invalid;
    d->startTime = QDateTime();
    d->endTime = QDateTime();
    d->relativeValue = 0;
    d->relativeUnit = TimeUnit::Days;
    d->includeLower = true;
    d->includeUpper = false;
    return *this;
}

bool TimeRangeFilter::isValid() const
{
    return d->mode != RangeMode::Invalid;
}

QPair<QDateTime, QDateTime> TimeRangeFilter::resolveTimeRange() const
{
    if (d->mode == RangeMode::Custom) {
        return qMakePair(d->startTime, d->endTime);
    }

    if (d->mode == RangeMode::Relative) {
        return resolveRelativeTimeRange(d->relativeValue, d->relativeUnit);
    }

    if (d->mode == RangeMode::FixedUnit) {
        return resolveFixedUnitTimeRange(d->relativeValue, d->relativeUnit);
    }

    return qMakePair(QDateTime(), QDateTime());
}

QPair<QDateTime, QDateTime> TimeRangeFilter::resolveRelativeTimeRange(int value, TimeUnit unit)
{
    // Relative mode: from N units ago to now (rolling window)
    QDateTime now = QDateTime::currentDateTime();
    QDateTime start;
    QDateTime end = now;

    switch (unit) {
    case TimeUnit::Minutes:
        // "Last N minutes": from N minutes ago to now
        start = now.addSecs(-value * 60);
        break;

    case TimeUnit::Hours:
        // "Last N hours": from N hours ago to now
        start = now.addSecs(-value * 3600);
        break;

    case TimeUnit::Days:
        // "Last N days": from N days ago (00:00:00) to now
        start = QDateTime(now.date().addDays(-value), QTime(0, 0, 0));
        break;

    case TimeUnit::Weeks:
        // "Last N weeks": from N weeks ago (Monday 00:00:00) to now
        {
            int daysToMonday = now.date().dayOfWeek() - 1;
            QDate thisMonday = now.date().addDays(-daysToMonday);
            QDate startMonday = thisMonday.addDays(-value * 7);
            start = QDateTime(startMonday, QTime(0, 0, 0));
        }
        break;

    case TimeUnit::Months:
        // "Last N months": from N months ago (1st day 00:00:00) to now
        {
            QDate firstOfThisMonth(now.date().year(), now.date().month(), 1);
            QDate startMonth = firstOfThisMonth.addMonths(-value);
            start = QDateTime(startMonth, QTime(0, 0, 0));
        }
        break;

    case TimeUnit::Years:
        // "Last N years": from N years ago (Jan 1st 00:00:00) to now
        {
            QDate startYear(now.date().year() - value, 1, 1);
            start = QDateTime(startYear, QTime(0, 0, 0));
        }
        break;
    }

    return qMakePair(start, end);
}

QPair<QDateTime, QDateTime> TimeRangeFilter::resolveFixedUnitTimeRange(int value, TimeUnit unit)
{
    // Fixed unit mode: complete unit range (today, yesterday, this week, etc.)
    QDateTime now = QDateTime::currentDateTime();
    QDateTime start;
    QDateTime end;

    switch (unit) {
    case TimeUnit::Minutes:
    case TimeUnit::Hours:
        // For minutes/hours, fixed unit doesn't make much sense, treat as relative
        return resolveRelativeTimeRange(value, unit);

    case TimeUnit::Days:
        if (value == 0) {
            // Today: from 00:00:00 today to 00:00:00 tomorrow
            start = QDateTime(now.date(), QTime(0, 0, 0));
            end = QDateTime(now.date().addDays(1), QTime(0, 0, 0));
        } else {
            // Yesterday/N days ago: complete day from 00:00:00 to 00:00:00 next day
            start = QDateTime(now.date().addDays(-value), QTime(0, 0, 0));
            end = QDateTime(now.date().addDays(-value + 1), QTime(0, 0, 0));
        }
        break;

    case TimeUnit::Weeks:
        if (value == 0) {
            // This week: from Monday 00:00:00 to next Monday 00:00:00
            int daysToMonday = now.date().dayOfWeek() - 1;
            QDate monday = now.date().addDays(-daysToMonday);
            start = QDateTime(monday, QTime(0, 0, 0));
            end = QDateTime(monday.addDays(7), QTime(0, 0, 0));
        } else {
            // Last week/N weeks ago: complete week
            int daysToMonday = now.date().dayOfWeek() - 1;
            QDate thisMonday = now.date().addDays(-daysToMonday);
            QDate startMonday = thisMonday.addDays(-value * 7);
            start = QDateTime(startMonday, QTime(0, 0, 0));
            end = QDateTime(startMonday.addDays(7), QTime(0, 0, 0));
        }
        break;

    case TimeUnit::Months:
        if (value == 0) {
            // This month: from 1st day 00:00:00 to 1st day of next month
            QDate firstOfMonth(now.date().year(), now.date().month(), 1);
            start = QDateTime(firstOfMonth, QTime(0, 0, 0));
            end = QDateTime(firstOfMonth.addMonths(1), QTime(0, 0, 0));
        } else {
            // Last month/N months ago: complete month
            QDate firstOfThisMonth(now.date().year(), now.date().month(), 1);
            QDate startMonth = firstOfThisMonth.addMonths(-value);
            start = QDateTime(startMonth, QTime(0, 0, 0));
            end = QDateTime(startMonth.addMonths(1), QTime(0, 0, 0));
        }
        break;

    case TimeUnit::Years:
        if (value == 0) {
            // This year: from Jan 1st 00:00:00 to Jan 1st of next year
            QDate firstOfYear(now.date().year(), 1, 1);
            start = QDateTime(firstOfYear, QTime(0, 0, 0));
            end = QDateTime(QDate(now.date().year() + 1, 1, 1), QTime(0, 0, 0));
        } else {
            // Last year/N years ago: complete year
            QDate startYear(now.date().year() - value, 1, 1);
            start = QDateTime(startYear, QTime(0, 0, 0));
            end = QDateTime(QDate(now.date().year() - value + 1, 1, 1), QTime(0, 0, 0));
        }
        break;
    }

    return qMakePair(start, end);
}

DFM_SEARCH_END_NS
