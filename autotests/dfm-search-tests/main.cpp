// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

// Test object creation functions are defined in their respective .cpp files
extern QObject *create_tst_DfmSearch();
extern QObject *create_tst_SearchUtils();
extern QObject *create_tst_TimeRangeFilter();
extern QObject *create_tst_TextSearchAPI();

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

    return result;
}
