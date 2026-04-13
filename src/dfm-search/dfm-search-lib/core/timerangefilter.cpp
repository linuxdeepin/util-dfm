// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/timerangefilter.h>

#include <QDate>

DFM_SEARCH_BEGIN_NS

TimeRangeFilter::TimeRangeFilter()
    : m_field(TimeField::ModifyTime)
    , m_mode(RangeMode::Invalid)
    , m_relativeValue(0)
    , m_relativeUnit(TimeUnit::Days)
    , m_includeLower(true)
    , m_includeUpper(false)
{
}

TimeRangeFilter &TimeRangeFilter::setTimeField(TimeField field)
{
    m_field = field;
    return *this;
}

TimeField TimeRangeFilter::timeField() const
{
    return m_field;
}

TimeRangeFilter &TimeRangeFilter::setLast(int value, TimeUnit unit)
{
    m_mode = RangeMode::Relative;  // Rolling range from N units ago to now
    m_relativeValue = value;
    m_relativeUnit = unit;
    m_startTime = QDateTime();
    m_endTime = QDateTime();
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setToday()
{
    m_mode = RangeMode::FixedUnit;  // Complete unit (today)
    m_relativeValue = 0;
    m_relativeUnit = TimeUnit::Days;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setYesterday()
{
    m_mode = RangeMode::FixedUnit;  // Complete unit (yesterday)
    m_relativeValue = 1;
    m_relativeUnit = TimeUnit::Days;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setThisWeek()
{
    m_mode = RangeMode::FixedUnit;
    m_relativeValue = 0;
    m_relativeUnit = TimeUnit::Weeks;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setLastWeek()
{
    m_mode = RangeMode::FixedUnit;
    m_relativeValue = 1;
    m_relativeUnit = TimeUnit::Weeks;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setThisMonth()
{
    m_mode = RangeMode::FixedUnit;
    m_relativeValue = 0;
    m_relativeUnit = TimeUnit::Months;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setLastMonth()
{
    m_mode = RangeMode::FixedUnit;
    m_relativeValue = 1;
    m_relativeUnit = TimeUnit::Months;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setThisYear()
{
    m_mode = RangeMode::FixedUnit;
    m_relativeValue = 0;
    m_relativeUnit = TimeUnit::Years;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setLastYear()
{
    m_mode = RangeMode::FixedUnit;
    m_relativeValue = 1;
    m_relativeUnit = TimeUnit::Years;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setRange(const QDateTime &start, const QDateTime &end)
{
    m_mode = RangeMode::Custom;
    m_startTime = start;
    m_endTime = end;
    m_relativeValue = 0;
    m_relativeUnit = TimeUnit::Days;
    return *this;
}

QDateTime TimeRangeFilter::startTime() const
{
    return m_startTime;
}

QDateTime TimeRangeFilter::endTime() const
{
    return m_endTime;
}

TimeRangeFilter &TimeRangeFilter::setIncludeLower(bool include)
{
    m_includeLower = include;
    return *this;
}

TimeRangeFilter &TimeRangeFilter::setIncludeUpper(bool include)
{
    m_includeUpper = include;
    return *this;
}

bool TimeRangeFilter::includeLower() const
{
    return m_includeLower;
}

bool TimeRangeFilter::includeUpper() const
{
    return m_includeUpper;
}

TimeRangeFilter &TimeRangeFilter::clear()
{
    m_mode = RangeMode::Invalid;
    m_startTime = QDateTime();
    m_endTime = QDateTime();
    m_relativeValue = 0;
    m_relativeUnit = TimeUnit::Days;
    m_includeLower = true;
    m_includeUpper = false;
    return *this;
}

bool TimeRangeFilter::isValid() const
{
    return m_mode != RangeMode::Invalid;
}

QPair<QDateTime, QDateTime> TimeRangeFilter::resolveTimeRange() const
{
    if (m_mode == RangeMode::Custom) {
        return qMakePair(m_startTime, m_endTime);
    }

    if (m_mode == RangeMode::Relative) {
        return resolveRelativeTimeRange(m_relativeValue, m_relativeUnit);
    }

    if (m_mode == RangeMode::FixedUnit) {
        return resolveFixedUnitTimeRange(m_relativeValue, m_relativeUnit);
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
