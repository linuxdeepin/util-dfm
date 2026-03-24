// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

class tst_DfmSearch : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void initialization_test();
};

void tst_DfmSearch::initTestCase()
{
    // Setup that runs once before all tests
}

void tst_DfmSearch::cleanupTestCase()
{
    // Cleanup that runs once after all tests
}

void tst_DfmSearch::initialization_test()
{
    // A basic test to verify the test framework is working
    QVERIFY(true);
}

QObject *create_tst_DfmSearch()
{
    return new tst_DfmSearch();
}

#include "tst_dfm_search.moc"
