// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

// Test object creation functions are defined in their respective .cpp files
extern QObject *create_tst_DfmSearch();
extern QObject *create_tst_SearchUtils();
extern QObject *create_tst_TimeRangeFilter();
extern QObject *create_tst_TextSearchAPI();
extern QObject *create_tst_RuleEngine();
extern QObject *create_tst_TimeExtraction();
extern QObject *create_tst_FileTypeExtraction();
extern QObject *create_tst_KeywordExtraction();
extern QObject *create_tst_ParsedIntent();
extern QObject *create_tst_ChineseNLP();
extern QObject *create_tst_SizeRangeFilter();

int main(int argc, char *argv[])
{
    int result = 0;

    // Run all test objects
    QObject *testObj1 = create_tst_DfmSearch();
    result |= QTest::qExec(testObj1, argc, argv);
    delete testObj1;

    QObject *testObj2 = create_tst_SearchUtils();
    result |= QTest::qExec(testObj2, argc, argv);
    delete testObj2;

    QObject *testObj3 = create_tst_TimeRangeFilter();
    result |= QTest::qExec(testObj3, argc, argv);
    delete testObj3;

    QObject *testObj4 = create_tst_TextSearchAPI();
    result |= QTest::qExec(testObj4, argc, argv);
    delete testObj4;

    QObject *testObj5 = create_tst_RuleEngine();
    result |= QTest::qExec(testObj5, argc, argv);
    delete testObj5;

    QObject *testObj6 = create_tst_TimeExtraction();
    result |= QTest::qExec(testObj6, argc, argv);
    delete testObj6;

    QObject *testObj7 = create_tst_FileTypeExtraction();
    result |= QTest::qExec(testObj7, argc, argv);
    delete testObj7;

    QObject *testObj8 = create_tst_KeywordExtraction();
    result |= QTest::qExec(testObj8, argc, argv);
    delete testObj8;

    QObject *testObj9 = create_tst_ParsedIntent();
    result |= QTest::qExec(testObj9, argc, argv);
    delete testObj9;

    QObject *testObj10 = create_tst_ChineseNLP();
    result |= QTest::qExec(testObj10, argc, argv);
    delete testObj10;

    QObject *testObj11 = create_tst_SizeRangeFilter();
    result |= QTest::qExec(testObj11, argc, argv);
    delete testObj11;

    return result;
}
