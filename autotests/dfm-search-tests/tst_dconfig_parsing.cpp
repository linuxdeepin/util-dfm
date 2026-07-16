// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QDebug>
#include <QVariant>
#include <QVariantList>
#include <QStringList>

#include <dfm-search/dsearch_global.h>
#include <dfm-search-lib/utils/searchutility.h>

using namespace DFMSEARCH;

class tst_DConfigParsing : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testSemicolonSeparatedString();
    void testSingleValueString();
    void testStringListVariant();
    void testVariantListVariant();
    void testEmptyString();
    void testEmptyStringList();
    void testInvalidVariant();
    void testNonConvertibleType();
    void testSemicolonWithEmptyParts();
    void testTrailingSemicolon();
};

void tst_DConfigParsing::initTestCase()
{
}

void tst_DConfigParsing::cleanupTestCase()
{
}

void tst_DConfigParsing::testSemicolonSeparatedString()
{
    QVariant value(QString("txt;pdf;docx;xlsx"));
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value for semicolon-separated string");
    QCOMPARE(result->size(), 4);
    QCOMPARE(result->at(0), QString("txt"));
    QCOMPARE(result->at(1), QString("pdf"));
    QCOMPARE(result->at(2), QString("docx"));
    QCOMPARE(result->at(3), QString("xlsx"));
}

void tst_DConfigParsing::testSingleValueString()
{
    QVariant value(QString("txt"));
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value for single string");
    QCOMPARE(result->size(), 1);
    QCOMPARE(result->at(0), QString("txt"));
}

void tst_DConfigParsing::testStringListVariant()
{
    QStringList list = { "txt", "pdf", "docx" };
    QVariant value(list);
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value for QStringList variant");
    QCOMPARE(result->size(), 3);
    QCOMPARE(result->at(0), QString("txt"));
    QCOMPARE(result->at(1), QString("pdf"));
    QCOMPARE(result->at(2), QString("docx"));
}

void tst_DConfigParsing::testVariantListVariant()
{
    QVariantList list;
    list << QVariant("home") << QVariant("usr") << QVariant("opt");
    QVariant value(list);
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value for QVariantList variant");
    QCOMPARE(result->size(), 3);
    QCOMPARE(result->at(0), QString("home"));
    QCOMPARE(result->at(1), QString("usr"));
    QCOMPARE(result->at(2), QString("opt"));
}

void tst_DConfigParsing::testEmptyString()
{
    QVariant value(QString(""));
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value for empty string");
    QCOMPARE(result->size(), 1);
    QVERIFY2(result->at(0).isEmpty(), "Single element should be empty string");
}

void tst_DConfigParsing::testEmptyStringList()
{
    QStringList list;
    QVariant value(list);
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value for empty QStringList");
    QCOMPARE(result->size(), 0);
}

void tst_DConfigParsing::testInvalidVariant()
{
    QVariant value;
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(!result.has_value(), "Should return nullopt for invalid QVariant");
}

void tst_DConfigParsing::testNonConvertibleType()
{
    QVariant value(42);
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(!result.has_value(), "Should return nullopt for non-convertible type (int)");
}

void tst_DConfigParsing::testSemicolonWithEmptyParts()
{
    QVariant value(QString("txt;;pdf;"));
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value");
    QCOMPARE(result->size(), 4);
    QCOMPARE(result->at(0), QString("txt"));
    QCOMPARE(result->at(1), QString(""));
    QCOMPARE(result->at(2), QString("pdf"));
    QCOMPARE(result->at(3), QString(""));
}

void tst_DConfigParsing::testTrailingSemicolon()
{
    QVariant value(QString("/home;/usr;/opt;"));
    auto result = SearchUtility::parseVariantToStringList(value);

    QVERIFY2(result.has_value(), "Should return a value");
    QCOMPARE(result->size(), 4);
    QCOMPARE(result->at(0), QString("/home"));
    QCOMPARE(result->at(1), QString("/usr"));
    QCOMPARE(result->at(2), QString("/opt"));
    QCOMPARE(result->at(3), QString(""));
}

QObject *create_tst_DConfigParsing()
{
    return new tst_DConfigParsing();
}

#include "tst_dconfig_parsing.moc"
