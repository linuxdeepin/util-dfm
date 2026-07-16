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
extern QObject *create_tst_IsSemanticQuery();
extern QObject *create_tst_SearchTarget();
extern QObject *create_tst_SemanticQueryBuilderTarget();
extern QObject *create_tst_LocationExtraction();
extern QObject *create_tst_ContentRetriever();
extern QObject *create_tst_ContentSearchEngine();
extern QObject *create_tst_FileNameSearchEngine();
extern QObject *create_tst_RecentSearchEngine();
extern QObject *create_tst_CliOptions();

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

    QObject *testObj12 = create_tst_IsSemanticQuery();
    result |= QTest::qExec(testObj12, argc, argv);
    delete testObj12;

    QObject *testObj13 = create_tst_SearchTarget();
    result |= QTest::qExec(testObj13, argc, argv);
    delete testObj13;

    QObject *testObj14 = create_tst_SemanticQueryBuilderTarget();
    result |= QTest::qExec(testObj14, argc, argv);
    delete testObj14;

    QObject *testObj14b = create_tst_LocationExtraction();
    result |= QTest::qExec(testObj14b, argc, argv);
    delete testObj14b;

    QObject *testObj15 = create_tst_ContentRetriever();
    result |= QTest::qExec(testObj15, argc, argv);
    delete testObj15;

    QObject *testObj16 = create_tst_ContentSearchEngine();
    result |= QTest::qExec(testObj16, argc, argv);
    delete testObj16;

    QObject *testObj17 = create_tst_FileNameSearchEngine();
    result |= QTest::qExec(testObj17, argc, argv);
    delete testObj17;

    QObject *testObj18 = create_tst_RecentSearchEngine();
    result |= QTest::qExec(testObj18, argc, argv);
    delete testObj18;

    QObject *testObj19 = create_tst_CliOptions();
    result |= QTest::qExec(testObj19, argc, argv);
    delete testObj19;

    return result;
}
