// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <dfm-search/sizerangefilter.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/searchresult.h>

#include "size_parser.h"

using namespace DFMSEARCH;

class tst_SizeRangeFilter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // SizeRangeFilter tests
    void testDefaultState();
    void testSetMin();
    void testSetMax();
    void testSetRange();
    void testFluentChaining();
    void testBoundaryControl();
    void testCopyConstructor();
    void testMoveConstructor();
    void testClear();
    void testIsValid();

    // SizeParser tests
    void testParseSizeBytes();
    void testParseSizeKilobytes();
    void testParseSizeMegabytes();
    void testParseSizeGigabytes();
    void testParseSizeTerabytes();
    void testParseSizeCaseInsensitive();
    void testParseSizeInvalid();
    void testParseSizeEmpty();
    void testParseSizeWithSpaces();

    // SearchOptions integration
    void testSearchOptionsSizeFilter();
    void testSearchOptionsClearSizeFilter();

    // FileNameResultAPI integration
    void testFileNameResultAPIFileSizeBytes();
};

// ==================== SizeRangeFilter Tests ====================

void tst_SizeRangeFilter::testDefaultState()
{
    SizeRangeFilter filter;
    QCOMPARE(filter.minSize(), 0);
    QCOMPARE(filter.maxSize(), 0);
    QCOMPARE(filter.includeLower(), true);
    QCOMPARE(filter.includeUpper(), true);
    QCOMPARE(filter.isValid(), false);
}

void tst_SizeRangeFilter::testSetMin()
{
    SizeRangeFilter filter;
    filter.setMin(1024);

    QCOMPARE(filter.minSize(), 1024);
    QCOMPARE(filter.isValid(), true);
}

void tst_SizeRangeFilter::testSetMax()
{
    SizeRangeFilter filter;
    filter.setMax(10 * 1024 * 1024);

    QCOMPARE(filter.maxSize(), 10 * 1024 * 1024);
    QCOMPARE(filter.isValid(), true);
}

void tst_SizeRangeFilter::testSetRange()
{
    SizeRangeFilter filter;
    filter.setRange(1024, 10 * 1024 * 1024);

    QCOMPARE(filter.minSize(), 1024);
    QCOMPARE(filter.maxSize(), 10 * 1024 * 1024);
    QCOMPARE(filter.isValid(), true);
}

void tst_SizeRangeFilter::testFluentChaining()
{
    SizeRangeFilter filter;
    auto &ref = filter.setMin(1024).setMax(10 * 1024 * 1024);

    QCOMPARE(filter.minSize(), 1024);
    QCOMPARE(filter.maxSize(), 10 * 1024 * 1024);
    QCOMPARE(&ref, &filter);   // 返回自身的引用
}

void tst_SizeRangeFilter::testBoundaryControl()
{
    SizeRangeFilter filter;
    filter.setRange(1024, 10 * 1024 * 1024);
    filter.setIncludeLower(false);
    filter.setIncludeUpper(false);

    QCOMPARE(filter.includeLower(), false);
    QCOMPARE(filter.includeUpper(), false);
}

void tst_SizeRangeFilter::testCopyConstructor()
{
    SizeRangeFilter original;
    original.setRange(1024, 10 * 1024 * 1024);
    original.setIncludeLower(false);

    SizeRangeFilter copy(original);
    QCOMPARE(copy.minSize(), 1024);
    QCOMPARE(copy.maxSize(), 10 * 1024 * 1024);
    QCOMPARE(copy.includeLower(), false);
    QCOMPARE(copy.includeUpper(), true);
    QCOMPARE(copy.isValid(), true);
}

void tst_SizeRangeFilter::testMoveConstructor()
{
    SizeRangeFilter original;
    original.setRange(1024, 10 * 1024 * 1024);

    SizeRangeFilter moved(std::move(original));
    QCOMPARE(moved.minSize(), 1024);
    QCOMPARE(moved.maxSize(), 10 * 1024 * 1024);
    QCOMPARE(moved.isValid(), true);
}

void tst_SizeRangeFilter::testClear()
{
    SizeRangeFilter filter;
    filter.setRange(1024, 10 * 1024 * 1024);
    filter.setIncludeLower(false);
    filter.setIncludeUpper(false);

    filter.clear();

    QCOMPARE(filter.minSize(), 0);
    QCOMPARE(filter.maxSize(), 0);
    QCOMPARE(filter.includeLower(), true);   // 重置为默认值
    QCOMPARE(filter.includeUpper(), true);
    QCOMPARE(filter.isValid(), false);
}

void tst_SizeRangeFilter::testIsValid()
{
    SizeRangeFilter filter;

    // 默认状态无效
    QCOMPARE(filter.isValid(), false);

    // 设置 min 后有效
    filter.setMin(1);
    QCOMPARE(filter.isValid(), true);
    filter.clear();

    // 设置 max 后有效
    filter.setMax(1);
    QCOMPARE(filter.isValid(), true);
    filter.clear();

    // 设置 0 值仍无效
    filter.setMin(0);
    QCOMPARE(filter.isValid(), false);
    filter.setMax(0);
    QCOMPARE(filter.isValid(), false);
}

// ==================== SizeParser Tests ====================

void tst_SizeRangeFilter::testParseSizeBytes()
{
    qint64 bytes = 0;

    QVERIFY(dfmsearch::SizeParser::parseSize("512", bytes));
    QCOMPARE(bytes, 512);

    QVERIFY(dfmsearch::SizeParser::parseSize("0", bytes));
    QCOMPARE(bytes, 0);

    QVERIFY(dfmsearch::SizeParser::parseSize("1024", bytes));
    QCOMPARE(bytes, 1024);
}

void tst_SizeRangeFilter::testParseSizeKilobytes()
{
    qint64 bytes = 0;

    QVERIFY(dfmsearch::SizeParser::parseSize("1K", bytes));
    QCOMPARE(bytes, 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1KB", bytes));
    QCOMPARE(bytes, 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("10K", bytes));
    QCOMPARE(bytes, 10240);

    QVERIFY(dfmsearch::SizeParser::parseSize("1.5K", bytes));
    QCOMPARE(bytes, 1536);
}

void tst_SizeRangeFilter::testParseSizeMegabytes()
{
    qint64 bytes = 0;

    QVERIFY(dfmsearch::SizeParser::parseSize("1M", bytes));
    QCOMPARE(bytes, 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1MB", bytes));
    QCOMPARE(bytes, 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("10M", bytes));
    QCOMPARE(bytes, 10LL * 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1.5M", bytes));
    QCOMPARE(bytes, static_cast<qint64>(1.5 * 1024 * 1024));
}

void tst_SizeRangeFilter::testParseSizeGigabytes()
{
    qint64 bytes = 0;

    QVERIFY(dfmsearch::SizeParser::parseSize("1G", bytes));
    QCOMPARE(bytes, 1024LL * 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1GB", bytes));
    QCOMPARE(bytes, 1024LL * 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("10G", bytes));
    QCOMPARE(bytes, 10LL * 1024 * 1024 * 1024);
}

void tst_SizeRangeFilter::testParseSizeTerabytes()
{
    qint64 bytes = 0;

    QVERIFY(dfmsearch::SizeParser::parseSize("1T", bytes));
    QCOMPARE(bytes, 1024LL * 1024 * 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1TB", bytes));
    QCOMPARE(bytes, 1024LL * 1024 * 1024 * 1024);
}

void tst_SizeRangeFilter::testParseSizeCaseInsensitive()
{
    qint64 bytes = 0;

    QVERIFY(dfmsearch::SizeParser::parseSize("1k", bytes));
    QCOMPARE(bytes, 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1m", bytes));
    QCOMPARE(bytes, 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1g", bytes));
    QCOMPARE(bytes, 1024LL * 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1kb", bytes));
    QCOMPARE(bytes, 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("1mb", bytes));
    QCOMPARE(bytes, 1024 * 1024);
}

void tst_SizeRangeFilter::testParseSizeInvalid()
{
    qint64 bytes = 0;

    // 未知后缀
    QVERIFY(!dfmsearch::SizeParser::parseSize("1X", bytes));

    // 纯字母
    QVERIFY(!dfmsearch::SizeParser::parseSize("abc", bytes));

    // 负数
    QVERIFY(!dfmsearch::SizeParser::parseSize("-1K", bytes));

    // 空数字
    QVERIFY(!dfmsearch::SizeParser::parseSize("K", bytes));
}

void tst_SizeRangeFilter::testParseSizeEmpty()
{
    qint64 bytes = -1;

    QVERIFY(!dfmsearch::SizeParser::parseSize("", bytes));
    QCOMPARE(bytes, -1);   // 不应被修改
}

void tst_SizeRangeFilter::testParseSizeWithSpaces()
{
    qint64 bytes = 0;

    QVERIFY(dfmsearch::SizeParser::parseSize("  1M  ", bytes));
    QCOMPARE(bytes, 1024 * 1024);

    QVERIFY(dfmsearch::SizeParser::parseSize("  512  ", bytes));
    QCOMPARE(bytes, 512);
}

// ==================== SearchOptions Integration Tests ====================

void tst_SizeRangeFilter::testSearchOptionsSizeFilter()
{
    SearchOptions options;

    // 默认无大小过滤
    QCOMPARE(options.hasSizeRangeFilter(), false);

    // 设置大小过滤
    SizeRangeFilter filter;
    filter.setRange(1024, 10 * 1024 * 1024);
    options.setSizeRangeFilter(filter);

    QCOMPARE(options.hasSizeRangeFilter(), true);

    SizeRangeFilter retrieved = options.sizeRangeFilter();
    QCOMPARE(retrieved.minSize(), 1024);
    QCOMPARE(retrieved.maxSize(), 10 * 1024 * 1024);
}

void tst_SizeRangeFilter::testSearchOptionsClearSizeFilter()
{
    SearchOptions options;

    SizeRangeFilter filter;
    filter.setRange(1024, 10 * 1024 * 1024);
    options.setSizeRangeFilter(filter);
    QCOMPARE(options.hasSizeRangeFilter(), true);

    options.clearSizeRangeFilter();
    QCOMPARE(options.hasSizeRangeFilter(), false);

    SizeRangeFilter retrieved = options.sizeRangeFilter();
    QCOMPARE(retrieved.minSize(), 0);
    QCOMPARE(retrieved.maxSize(), 0);
}

// ==================== FileNameResultAPI Integration Tests ====================

void tst_SizeRangeFilter::testFileNameResultAPIFileSizeBytes()
{
    SearchResult result("/home/user/test.txt");
    FileNameResultAPI api(result);

    // 默认值
    QCOMPARE(api.fileSizeBytes(), 0);

    // 设置和获取
    api.setFileSizeBytes(1024);
    QCOMPARE(api.fileSizeBytes(), 1024);

    // 设置大文件
    api.setFileSizeBytes(10LL * 1024 * 1024 * 1024);
    QCOMPARE(api.fileSizeBytes(), 10LL * 1024 * 1024 * 1024);

    // 设置 0
    api.setFileSizeBytes(0);
    QCOMPARE(api.fileSizeBytes(), 0);
}

QObject *create_tst_SizeRangeFilter()
{
    return new tst_SizeRangeFilter();
}

#include "tst_size_range_filter.moc"
