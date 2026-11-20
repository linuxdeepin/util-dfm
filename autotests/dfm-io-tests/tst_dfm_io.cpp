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
    // 同一inode经不同路径监视（如UOS上/home是/data/home的bind挂载）时，不能把为A路径
    // 建的寄存监视器复用给B路径：GLib用"建监视时的路径字符串"拼事件child路径，跨路径
    // 复用会让事件带着旧前缀上报（如监视/data/...却报file:///home/...），上层按当前
    // 视图URL匹配不上，表现为监视器失效。用symlink构造两个路径精确复现。
    void parkedMonitorNotReusedAcrossPaths();
    void watcherRecoversAfterAllPathsStopped();
    // 文件管理器"主目录 -> 数据盘 -> 主目录"的往返：新视图的监视器总是先于旧视图销毁而
    // 建立，旧视图stop时同inode仍有存活者。若此时cancel掉共享的内核watch描述符，存活者
    // 到GLib的missing轮询（4秒）之前都收不到事件，切回主目录后表现为监视器失效。
    void watcherSurvivesRoundTripBetweenSameInodePaths();
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

void tst_DfmIO::parkedMonitorNotReusedAcrossPaths()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString root = tmpDir.path();
    const QString link = root + "/sibling-link";
    QVERIFY(QFile::link(root, link));

    // 先经root路径建监视器，再由link路径建同inode的监视器，使root监视器可被寄存
    DWatcher rootWatcher(QUrl::fromLocalFile(root));
    QVERIFY(rootWatcher.start());

    DWatcher linkWatcher(QUrl::fromLocalFile(link));
    QVERIFY(linkWatcher.start());

    // 停掉root路径的监视器：其监视器进入寄存桶（不能cancel，否则link侧失聪）
    QVERIFY(rootWatcher.stop());

    // 为link路径重新建监视器：必须新建，不能复用登记在root路径名下的寄存监视器
    DWatcher revivedWatcher(QUrl::fromLocalFile(link));
    QVERIFY(revivedWatcher.start());

    QSignalSpy added(&revivedWatcher, &DWatcher::fileAdded);
    QVERIFY(createEmptyFile(link + "/after-revive.txt"));

    QVERIFY(added.wait(3000));
    // 事件URL必须落在本监视器自己的路径下（link），不能带root前缀
    const QUrl reported = added.first().at(0).toUrl();
    QCOMPARE(reported, QUrl::fromLocalFile(link + "/after-revive.txt"));

    QVERIFY(revivedWatcher.stop());
    QVERIFY(linkWatcher.stop());
    QFile::remove(link + "/after-revive.txt");
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

void tst_DfmIO::watcherSurvivesRoundTripBetweenSameInodePaths()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString root = tmpDir.path();
    const QString link = root + "/sibling-link";
    QVERIFY(QFile::link(root, link));

    // 同inode的两条路径都会收到事件（各带自己的路径前缀），只关心"自己前缀"那条
    auto received = [](const QSignalSpy &spy, const QString &path) {
        for (const QList<QVariant> &args : spy) {
            if (!args.isEmpty() && args.first().toUrl() == QUrl::fromLocalFile(path))
                return true;
        }
        return false;
    };

    // 打开主目录视图
    DWatcher homeWatcher(QUrl::fromLocalFile(root));
    QVERIFY(homeWatcher.start());

    // 切到数据盘视图（同inode的另一条路径），随后旧的主目录视图被销毁
    DWatcher dataWatcher(QUrl::fromLocalFile(link));
    QVERIFY(dataWatcher.start());
    QSignalSpy dataAdded(&dataWatcher, &DWatcher::fileAdded);
    QVERIFY(homeWatcher.stop());

    // 数据盘视图必须仍能收到事件
    QVERIFY(createEmptyFile(link + "/data-view.txt"));
    QVERIFY(dataAdded.wait(3000));
    QVERIFY(received(dataAdded, link + "/data-view.txt"));

    // 切回主目录视图，随后数据盘视图被销毁
    DWatcher backWatcher(QUrl::fromLocalFile(root));
    QVERIFY(backWatcher.start());
    QSignalSpy backAdded(&backWatcher, &DWatcher::fileAdded);
    QVERIFY(dataWatcher.stop());

    // 回到主目录后监视器不能失效
    QVERIFY(createEmptyFile(root + "/home-view.txt"));
    QVERIFY(backAdded.wait(3000));
    QVERIFY(received(backAdded, root + "/home-view.txt"));

    QVERIFY(backWatcher.stop());
    QFile::remove(link + "/data-view.txt");
    QFile::remove(root + "/home-view.txt");
}

QTEST_MAIN(tst_DfmIO)
#include "tst_dfm_io.moc"
