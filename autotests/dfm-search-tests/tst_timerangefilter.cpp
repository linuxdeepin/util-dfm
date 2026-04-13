// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <QTest>
#include <QDateTime>
#include <QDate>
#include <QTime>

#include <dfm-search/timerangefilter.h>

using namespace DFMSEARCH;

class tst_TimeRangeFilter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void default_constructor_test();
    void set_time_field_test();
    void set_last_test();
    void set_today_test();
    void set_yesterday_test();
    void set_this_week_test();
    void set_last_week_test();
    void set_this_month_test();
    void set_last_month_test();
    void set_this_year_test();
    void set_last_year_test();
    void set_range_test();
    void boundary_control_test();
    void clear_test();
    void is_valid_test();
    void resolve_time_range_test();
    void fluent_interface_test();
};

void tst_TimeRangeFilter::initTestCase()
{
}

void tst_TimeRangeFilter::cleanupTestCase()
{
}

void tst_TimeRangeFilter::default_constructor_test()
{
    TimeRangeFilter filter;
    QVERIFY(!filter.isValid());
    QCOMPARE(filter.timeField(), TimeField::ModifyTime);
    QVERIFY(filter.includeLower());
    QVERIFY(!filter.includeUpper());
}

void tst_TimeRangeFilter::set_time_field_test()
{
    TimeRangeFilter filter;
    filter.setTimeField(TimeField::BirthTime);
    QCOMPARE(filter.timeField(), TimeField::BirthTime);

    filter.setTimeField(TimeField::ModifyTime);
    QCOMPARE(filter.timeField(), TimeField::ModifyTime);
}

void tst_TimeRangeFilter::set_last_test()
{
    TimeRangeFilter filter;
    filter.setLast(3, TimeUnit::Days);
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // "Last 3 days" for setLast: rolling range from 3 days ago (00:00:00) to now
    QDateTime now = QDateTime::currentDateTime();
    QDateTime expectedStart = QDateTime(now.date().addDays(-3), QTime(0, 0, 0));
    QCOMPARE(start, expectedStart);
    // End should be close to now (within 2 seconds)
    QVERIFY(qAbs(end.secsTo(now)) < 2);

    // Test minutes - this is precise
    filter.setLast(30, TimeUnit::Minutes);
    auto [start2, end2] = filter.resolveTimeRange();
    QVERIFY(start2.isValid());
    QVERIFY(end2.isValid());
    // Minutes should be precise (within 2 seconds tolerance)
    QVERIFY(qAbs(start2.secsTo(now.addSecs(-30 * 60))) < 2);

    // Test hours - this is precise
    filter.setLast(2, TimeUnit::Hours);
    auto [start3, end3] = filter.resolveTimeRange();
    QVERIFY(start3.isValid());
    QVERIFY(end3.isValid());
    // Hours should be precise (within 2 seconds tolerance)
    QVERIFY(qAbs(start3.secsTo(now.addSecs(-2 * 3600))) < 2);
}

void tst_TimeRangeFilter::set_today_test()
{
    TimeRangeFilter filter;
    filter.setToday();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Today should start at 00:00:00
    QCOMPARE(start.time(), QTime(0, 0, 0));
    QCOMPARE(start.date(), QDate::currentDate());

    // End should be tomorrow 00:00:00
    QCOMPARE(end.date(), QDate::currentDate().addDays(1));
    QCOMPARE(end.time(), QTime(0, 0, 0));
}

void tst_TimeRangeFilter::set_yesterday_test()
{
    TimeRangeFilter filter;
    filter.setYesterday();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Yesterday should start at 00:00:00
    QCOMPARE(start.time(), QTime(0, 0, 0));
    QCOMPARE(start.date(), QDate::currentDate().addDays(-1));

    // End should be today 00:00:00
    QCOMPARE(end.date(), QDate::currentDate());
    QCOMPARE(end.time(), QTime(0, 0, 0));
}

void tst_TimeRangeFilter::set_this_week_test()
{
    TimeRangeFilter filter;
    filter.setThisWeek();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Should start on Monday 00:00:00
    QCOMPARE(start.time(), QTime(0, 0, 0));
    QCOMPARE(start.date().dayOfWeek(), 1);   // Monday is day 1
}

void tst_TimeRangeFilter::set_last_week_test()
{
    TimeRangeFilter filter;
    filter.setLastWeek();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Should be a 7-day range
    qint64 diff = start.daysTo(end);
    QCOMPARE(diff, 7);
}

void tst_TimeRangeFilter::set_this_month_test()
{
    TimeRangeFilter filter;
    filter.setThisMonth();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Should start on the 1st
    QCOMPARE(start.date().day(), 1);
    QCOMPARE(start.time(), QTime(0, 0, 0));
}

void tst_TimeRangeFilter::set_last_month_test()
{
    TimeRangeFilter filter;
    filter.setLastMonth();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Should start on the 1st of last month
    QCOMPARE(start.date().day(), 1);
    QCOMPARE(start.time(), QTime(0, 0, 0));
}

void tst_TimeRangeFilter::set_this_year_test()
{
    TimeRangeFilter filter;
    filter.setThisYear();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Should start on Jan 1st
    QCOMPARE(start.date().month(), 1);
    QCOMPARE(start.date().day(), 1);
    QCOMPARE(start.time(), QTime(0, 0, 0));
}

void tst_TimeRangeFilter::set_last_year_test()
{
    TimeRangeFilter filter;
    filter.setLastYear();
    QVERIFY(filter.isValid());

    auto [start, end] = filter.resolveTimeRange();
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());

    // Should be previous year
    QCOMPARE(start.date().year(), QDate::currentDate().year() - 1);
}

void tst_TimeRangeFilter::set_range_test()
{
    TimeRangeFilter filter;
    QDateTime startDate(QDate(2025, 10, 1), QTime(0, 0, 0));
    QDateTime endDate(QDate(2025, 10, 31), QTime(23, 59, 59));

    filter.setRange(startDate, endDate);
    QVERIFY(filter.isValid());

    QCOMPARE(filter.startTime(), startDate);
    QCOMPARE(filter.endTime(), endDate);

    auto [resolvedStart, resolvedEnd] = filter.resolveTimeRange();
    QCOMPARE(resolvedStart, startDate);
    QCOMPARE(resolvedEnd, endDate);
}

void tst_TimeRangeFilter::boundary_control_test()
{
    TimeRangeFilter filter;
    filter.setToday();

    // Default: includeLower=true, includeUpper=false
    QVERIFY(filter.includeLower());
    QVERIFY(!filter.includeUpper());

    // Test fluent interface
    filter.setIncludeLower(false).setIncludeUpper(true);
    QVERIFY(!filter.includeLower());
    QVERIFY(filter.includeUpper());
}

void tst_TimeRangeFilter::clear_test()
{
    TimeRangeFilter filter;
    filter.setToday();
    QVERIFY(filter.isValid());

    filter.clear();
    QVERIFY(!filter.isValid());
    QCOMPARE(filter.timeField(), TimeField::ModifyTime);
    QVERIFY(filter.includeLower());
    QVERIFY(!filter.includeUpper());
}

void tst_TimeRangeFilter::is_valid_test()
{
    TimeRangeFilter filter;
    QVERIFY(!filter.isValid());

    filter.setToday();
    QVERIFY(filter.isValid());

    filter.clear();
    QVERIFY(!filter.isValid());

    QDateTime start, end;
    filter.setRange(start, end);
    // Range with invalid datetimes is still "valid" in terms of being set
    QVERIFY(filter.isValid());
}

void tst_TimeRangeFilter::resolve_time_range_test()
{
    // Test static method
    auto [start, end] = TimeRangeFilter::resolveRelativeTimeRange(0, TimeUnit::Days);
    QVERIFY(start.isValid());
    QVERIFY(end.isValid());
    QCOMPARE(start.time(), QTime(0, 0, 0));

    // Test with different units
    auto [start2, end2] = TimeRangeFilter::resolveRelativeTimeRange(30, TimeUnit::Minutes);
    QVERIFY(start2.isValid());
    QVERIFY(end2.isValid());
}

void tst_TimeRangeFilter::fluent_interface_test()
{
    // Test method chaining
    TimeRangeFilter filter;
    filter.setTimeField(TimeField::BirthTime)
          .setLast(7, TimeUnit::Days)
          .setIncludeLower(true)
          .setIncludeUpper(true);

    QCOMPARE(filter.timeField(), TimeField::BirthTime);
    QVERIFY(filter.isValid());
    QVERIFY(filter.includeLower());
    QVERIFY(filter.includeUpper());

    // Test clear returns reference
    filter.clear();
    QVERIFY(!filter.isValid());
}

QObject *create_tst_TimeRangeFilter()
{
    return new tst_TimeRangeFilter();
}

#include "tst_timerangefilter.moc"
