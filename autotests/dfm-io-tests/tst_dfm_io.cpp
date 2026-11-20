// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QThread>
#include <QDeadlineTimer>
#include <QProcess>
#include <QSignalSpy>

#include <dfm-io/dwatcher.h>

USING_IO_NAMESPACE

class tst_DfmIO : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void initialization_test();

    // GLib inotify后端按inode共享watch描述符：对同一inode的两条路径分别建监视器后
    // cancel其一，存活者会永久收不到事件。用symlink构造同inode双路径精确复现
    // （g_file_new_for_path不解析symlink，两条路径各自建订阅、内核共享wd；
    // 不能用"/dir/."作第二路径——g_file_new_for_path会把它canonicalize成"/dir"）。
    void stopOnePathKeepsOtherPathMonitored();
    void watcherRecoversAfterAllPathsStopped();
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

static bool createEmptyFile(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly);
}

void tst_DfmIO::stopOnePathKeepsOtherPathMonitored()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString root = tmpDir.path();
    const QString link = root + "/sibling-link";
    QVERIFY(QFile::link(root, link));

    DWatcher rootWatcher(QUrl::fromLocalFile(root));
    DWatcher linkWatcher(QUrl::fromLocalFile(link));
    QVERIFY(rootWatcher.start());
    QVERIFY(linkWatcher.start());

    QSignalSpy rootAdded(&rootWatcher, &DWatcher::fileAdded);
    QSignalSpy linkAdded(&linkWatcher, &DWatcher::fileAdded);

    // 停掉经由symlink路径的监视器：不应cancel共享订阅
    QVERIFY(linkWatcher.stop());

    QVERIFY(createEmptyFile(root + "/after-stop.txt"));

    // root监视器应收到created事件
    QVERIFY(rootAdded.wait(3000));
    QCOMPARE(linkAdded.count(), 0);   // 已停止的监视器不得收事件

    QVERIFY(rootWatcher.stop());
    QFile::remove(root + "/after-stop.txt");
}

void tst_DfmIO::watcherRecoversAfterAllPathsStopped()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString root = tmpDir.path();
    const QString link = root + "/sibling-link";
    QVERIFY(QFile::link(root, link));

    {
        DWatcher rootWatcher(QUrl::fromLocalFile(root));
        DWatcher linkWatcher(QUrl::fromLocalFile(link));
        QVERIFY(rootWatcher.start());
        QVERIFY(linkWatcher.start());
        QVERIFY(linkWatcher.stop());
        QVERIFY(rootWatcher.stop());
    }   // 全部停止：最后一个stop应cancel并析构寄存的监视器，GLib状态自净

    DWatcher freshWatcher(QUrl::fromLocalFile(root));
    QVERIFY(freshWatcher.start());
    QSignalSpy freshAdded(&freshWatcher, &DWatcher::fileAdded);

    QVERIFY(createEmptyFile(root + "/after-all-stopped.txt"));
    QVERIFY(freshAdded.wait(3000));

    QVERIFY(freshWatcher.stop());
    QFile::remove(root + "/after-all-stopped.txt");
}

QTEST_MAIN(tst_DfmIO)
#include "tst_dfm_io.moc"
