// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

class tst_DfmIO : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void initialization_test();
};

void tst_DfmIO::initTestCase()
{
    // Setup that runs once before all tests
}

void tst_DfmIO::cleanupTestCase()
{
    // Cleanup that runs once after all tests
}

void tst_DfmIO::initialization_test()
{
    // A basic test to verify the test framework is working
    QVERIFY(true);
}

QTEST_MAIN(tst_DfmIO)
#include "tst_dfm_io.moc"
