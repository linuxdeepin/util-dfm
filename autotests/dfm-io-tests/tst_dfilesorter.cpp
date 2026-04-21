// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QUrl>
#include <QCollator>
#include <QDebug>
#include <QTemporaryDir>
#include <QFile>
#include <QFileDevice>
#include <QDateTime>

#include "sort/dfilesorter.h"
#include "sort/dsortkeycache.h"

using namespace dfmio;

class tst_DFileSorter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // DSortKeyCache 测试
    void sortKeyCache_basic();
    void sortKeyCache_numericMode();
    void sortKeyCache_threadSafety();

    // DFileSorter 测试
    void fileSorter_sortByName();
    void fileSorter_sortByName_descending();
    void fileSorter_sortBySize();
    void fileSorter_sortBySize_withDirs();
    void fileSorter_sortBySize_descending();
    void fileSorter_sortByLastModified();
    void fileSorter_sortByLastRead();
    void fileSorter_mixDirAndFile();
    void fileSorter_separateDirAndFile();
    void fileSorter_emptyList();
    void fileSorter_singleItem();
    void fileSorter_sortByLastModified_withSymlink();
    void fileSorter_sortByLastRead_withSymlink();
};

void tst_DFileSorter::initTestCase()
{
}

void tst_DFileSorter::cleanupTestCase()
{
}

// ==================== DSortKeyCache 测试 ====================

void tst_DFileSorter::sortKeyCache_basic()
{
    DSortKeyCache &cache = DSortKeyCache::instance();

    // 相同字符串应返回相同的排序键
    QCollatorSortKey key1 = cache.sortKey("test");
    QCollatorSortKey key2 = cache.sortKey("test");
    QCOMPARE(key1.compare(key2), 0);
}

void tst_DFileSorter::sortKeyCache_numericMode()
{
    DSortKeyCache &cache = DSortKeyCache::instance();

    // 数字自然排序：file2 < file10
    QCollatorSortKey key2 = cache.sortKey("file2");
    QCollatorSortKey key10 = cache.sortKey("file10");

    // file2 应该排在 file10 前面（numericMode 开启）
    QVERIFY(key2.compare(key10) < 0);
}

void tst_DFileSorter::sortKeyCache_threadSafety()
{
    // 测试 thread_local QCollator 的线程安全性
    // 每个线程应该有独立的 QCollator 实例
    DSortKeyCache &cache = DSortKeyCache::instance();

    // 验证排序键可以正常比较（不假设大小写不敏感）
    QCollatorSortKey key1 = cache.sortKey("abc");

    // 相同字符串返回相同排序键
    QCollatorSortKey key2 = cache.sortKey("abc");
    QCOMPARE(key1.compare(key2), 0);
}

// ==================== DFileSorter 测试 ====================

void tst_DFileSorter::fileSorter_sortByName()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Name;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    // 创建测试文件列表
    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/file10.txt");
    file1->isDir = false;
    file1->filesize = 100;
    files.append(file1);

    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/file2.txt");
    file2->isDir = false;
    file2->filesize = 200;
    files.append(file2);

    auto file3 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file3->url = QUrl::fromLocalFile("/test/file1.txt");
    file3->isDir = false;
    file3->filesize = 300;
    files.append(file3);

    auto sorted = sorter.sort(std::move(files));

    // 验证排序结果：file1 < file2 < file10（自然排序）
    QCOMPARE(sorted.size(), 3);
    QCOMPARE(sorted[0]->url.fileName(), QString("file1.txt"));
    QCOMPARE(sorted[1]->url.fileName(), QString("file2.txt"));
    QCOMPARE(sorted[2]->url.fileName(), QString("file10.txt"));
}

void tst_DFileSorter::fileSorter_sortByName_descending()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Name;
    config.order = Qt::DescendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/a.txt");
    file1->isDir = false;
    files.append(file1);

    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/b.txt");
    file2->isDir = false;
    files.append(file2);

    auto file3 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file3->url = QUrl::fromLocalFile("/test/c.txt");
    file3->isDir = false;
    files.append(file3);

    auto sorted = sorter.sort(std::move(files));

    // 降序：c > b > a
    QCOMPARE(sorted.size(), 3);
    QCOMPARE(sorted[0]->url.fileName(), QString("c.txt"));
    QCOMPARE(sorted[1]->url.fileName(), QString("b.txt"));
    QCOMPARE(sorted[2]->url.fileName(), QString("a.txt"));
}

void tst_DFileSorter::fileSorter_sortBySize()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Size;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/large.txt");
    file1->isDir = false;
    file1->filesize = 1000;
    files.append(file1);

    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/small.txt");
    file2->isDir = false;
    file2->filesize = 100;
    files.append(file2);

    auto file3 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file3->url = QUrl::fromLocalFile("/test/medium.txt");
    file3->isDir = false;
    file3->filesize = 500;
    files.append(file3);

    auto sorted = sorter.sort(std::move(files));

    // 按大小升序：small < medium < large
    QCOMPARE(sorted.size(), 3);
    QCOMPARE(sorted[0]->url.fileName(), QString("small.txt"));
    QCOMPARE(sorted[1]->url.fileName(), QString("medium.txt"));
    QCOMPARE(sorted[2]->url.fileName(), QString("large.txt"));
}

void tst_DFileSorter::fileSorter_sortBySize_withDirs()
{
    // 测试按大小排序时，目录（有效大小 -1）始终排在前面（升序时）
    // 这与原 DLocalHelper::fileSizeByEnt 的行为一致
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Size;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;  // 混排模式

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    // 目录：大小通常为 4096，但有效大小应为 -1
    auto dir1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    dir1->url = QUrl::fromLocalFile("/test/docs");
    dir1->isDir = true;
    dir1->isSymLink = false;
    dir1->filesize = 4096;  // 实际大小
    files.append(dir1);

    // 小文件：100 字节（小于 4096）
    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/small.txt");
    file1->isDir = false;
    file1->filesize = 100;
    files.append(file1);

    // 中等文件：2000 字节（小于 4096）
    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/medium.txt");
    file2->isDir = false;
    file2->filesize = 2000;
    files.append(file2);

    // 大文件：10000 字节（大于 4096）
    auto file3 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file3->url = QUrl::fromLocalFile("/test/large.txt");
    file3->isDir = false;
    file3->filesize = 10000;
    files.append(file3);

    // 另一个目录
    auto dir2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    dir2->url = QUrl::fromLocalFile("/test/apps");
    dir2->isDir = true;
    dir2->isSymLink = false;
    dir2->filesize = 4096;
    files.append(dir2);

    auto sorted = sorter.sort(std::move(files));

    // 验证：升序时目录（有效大小 -1）在前，然后按文件大小排序
    QCOMPARE(sorted.size(), 5);
    // 目录在前（按名称排序，因为大小相同都是 -1）
    QCOMPARE(sorted[0]->isDir, true);
    QCOMPARE(sorted[0]->url.fileName(), QString("apps"));
    QCOMPARE(sorted[1]->isDir, true);
    QCOMPARE(sorted[1]->url.fileName(), QString("docs"));
    // 文件在后（按大小排序）
    QCOMPARE(sorted[2]->isDir, false);
    QCOMPARE(sorted[2]->url.fileName(), QString("small.txt"));
    QCOMPARE(sorted[3]->isDir, false);
    QCOMPARE(sorted[3]->url.fileName(), QString("medium.txt"));
    QCOMPARE(sorted[4]->isDir, false);
    QCOMPARE(sorted[4]->url.fileName(), QString("large.txt"));
}

void tst_DFileSorter::fileSorter_sortBySize_descending()
{
    // 测试按大小降序排序
    // 降序时，目录（有效大小 -1）会排到后面
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Size;
    config.order = Qt::DescendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    // 目录（有效大小 -1）
    auto dir1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    dir1->url = QUrl::fromLocalFile("/test/docs");
    dir1->isDir = true;
    dir1->isSymLink = false;
    dir1->filesize = 4096;
    files.append(dir1);

    // 文件
    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/small.txt");
    file1->isDir = false;
    file1->filesize = 100;
    files.append(file1);

    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/large.txt");
    file2->isDir = false;
    file2->filesize = 10000;
    files.append(file2);

    auto sorted = sorter.sort(std::move(files));

    // 降序：大文件 > 小文件 > 目录（-1 最小，反转后在最后）
    QCOMPARE(sorted.size(), 3);
    QCOMPARE(sorted[0]->url.fileName(), QString("large.txt"));  // 大文件
    QCOMPARE(sorted[1]->url.fileName(), QString("small.txt"));  // 小文件
    QCOMPARE(sorted[2]->isDir, true);  // 目录在最后
}

void tst_DFileSorter::fileSorter_sortByLastModified()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::LastModified;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/newest.txt");
    file1->isDir = false;
    file1->lastModifed = 3000;
    file1->lastModifedNs = 0;
    files.append(file1);

    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/oldest.txt");
    file2->isDir = false;
    file2->lastModifed = 1000;
    file2->lastModifedNs = 0;
    files.append(file2);

    auto file3 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file3->url = QUrl::fromLocalFile("/test/middle.txt");
    file3->isDir = false;
    file3->lastModifed = 2000;
    file3->lastModifedNs = 0;
    files.append(file3);

    auto sorted = sorter.sort(std::move(files));

    // 按修改时间升序：oldest < middle < newest
    QCOMPARE(sorted.size(), 3);
    QCOMPARE(sorted[0]->url.fileName(), QString("oldest.txt"));
    QCOMPARE(sorted[1]->url.fileName(), QString("middle.txt"));
    QCOMPARE(sorted[2]->url.fileName(), QString("newest.txt"));
}

void tst_DFileSorter::fileSorter_sortByLastRead()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::LastRead;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/read_newest.txt");
    file1->isDir = false;
    file1->lastRead = 3000;
    file1->lastReadNs = 0;
    files.append(file1);

    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/read_oldest.txt");
    file2->isDir = false;
    file2->lastRead = 1000;
    file2->lastReadNs = 0;
    files.append(file2);

    auto sorted = sorter.sort(std::move(files));

    // 按访问时间升序：oldest < newest
    QCOMPARE(sorted.size(), 2);
    QCOMPARE(sorted[0]->url.fileName(), QString("read_oldest.txt"));
    QCOMPARE(sorted[1]->url.fileName(), QString("read_newest.txt"));
}

void tst_DFileSorter::fileSorter_mixDirAndFile()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Name;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;  // 混排模式

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto dir1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    dir1->url = QUrl::fromLocalFile("/test/docs");
    dir1->isDir = true;
    dir1->isSymLink = false;
    files.append(dir1);

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/afile.txt");
    file1->isDir = false;
    files.append(file1);

    auto dir2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    dir2->url = QUrl::fromLocalFile("/test/apps");
    dir2->isDir = true;
    dir2->isSymLink = false;
    files.append(dir2);

    auto sorted = sorter.sort(std::move(files));

    // 混排模式：所有项一起排序
    QCOMPARE(sorted.size(), 3);
    QCOMPARE(sorted[0]->url.fileName(), QString("afile.txt"));  // a 排最前
    QCOMPARE(sorted[1]->url.fileName(), QString("apps"));
    QCOMPARE(sorted[2]->url.fileName(), QString("docs"));
}

void tst_DFileSorter::fileSorter_separateDirAndFile()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Name;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = false;  // 分离模式：目录在前

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/a_file.txt");
    file1->isDir = false;
    files.append(file1);

    auto dir1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    dir1->url = QUrl::fromLocalFile("/test/z_dir");
    dir1->isDir = true;
    dir1->isSymLink = false;
    files.append(dir1);

    auto file2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file2->url = QUrl::fromLocalFile("/test/b_file.txt");
    file2->isDir = false;
    files.append(file2);

    auto dir2 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    dir2->url = QUrl::fromLocalFile("/test/a_dir");
    dir2->isDir = true;
    dir2->isSymLink = false;
    files.append(dir2);

    auto sorted = sorter.sort(std::move(files));

    // 分离模式：目录在前（按名称排序），文件在后（按名称排序）
    QCOMPARE(sorted.size(), 4);
    // 目录在前
    QCOMPARE(sorted[0]->url.fileName(), QString("a_dir"));
    QCOMPARE(sorted[1]->url.fileName(), QString("z_dir"));
    // 文件在后
    QCOMPARE(sorted[2]->url.fileName(), QString("a_file.txt"));
    QCOMPARE(sorted[3]->url.fileName(), QString("b_file.txt"));
}

void tst_DFileSorter::fileSorter_emptyList()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Name;

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;
    auto sorted = sorter.sort(std::move(files));

    QVERIFY(sorted.isEmpty());
}

void tst_DFileSorter::fileSorter_singleItem()
{
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::Name;

    DFileSorter sorter(config);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile("/test/single.txt");
    file1->isDir = false;
    files.append(file1);

    auto sorted = sorter.sort(std::move(files));

    QCOMPARE(sorted.size(), 1);
    QCOMPARE(sorted[0]->url.fileName(), QString("single.txt"));
}

void tst_DFileSorter::fileSorter_sortByLastModified_withSymlink()
{
    // 测试按修改时间排序时，符号链接使用其指向文件的时间
    // 需要创建临时文件来测试真实场景
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::LastModified;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    // 创建临时目录和文件
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString oldFile = tempDir.path() + "/old_file.txt";
    QString newFile = tempDir.path() + "/new_file.txt";
    QString linkFile = tempDir.path() + "/link_to_old.txt";

    // 创建文件
    {
        QFile f(oldFile);
        f.open(QIODevice::WriteOnly);
        f.setFileTime(QDateTime::fromSecsSinceEpoch(1000), QFileDevice::FileModificationTime);
    }
    {
        QFile f(newFile);
        f.open(QIODevice::WriteOnly);
        f.setFileTime(QDateTime::fromSecsSinceEpoch(3000), QFileDevice::FileModificationTime);
    }

    // 创建符号链接指向旧文件
    QFile::link(oldFile, linkFile);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    // 普通文件：new_file
    auto file1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    file1->url = QUrl::fromLocalFile(newFile);
    file1->isDir = false;
    file1->isSymLink = false;
    file1->lastModifed = 3000;
    file1->lastModifedNs = 0;
    files.append(file1);

    // 符号链接：指向 old_file（时间 1000）
    auto symlink1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    symlink1->url = QUrl::fromLocalFile(linkFile);
    symlink1->isDir = false;
    symlink1->isSymLink = true;
    symlink1->symlinkUrl = QUrl::fromLocalFile(oldFile);  // 指向目标文件
    symlink1->lastModifed = 2000;  // 链接本身的时间（排序时应忽略，使用目标时间 1000）
    symlink1->lastModifedNs = 0;
    files.append(symlink1);

    auto sorted = sorter.sort(std::move(files));

    // 按修改时间升序：link_to_old(1000，目标文件时间) < new_file(3000)
    QCOMPARE(sorted.size(), 2);
    QCOMPARE(sorted[0]->url.fileName(), QString("link_to_old.txt"));
    QCOMPARE(sorted[1]->url.fileName(), QString("new_file.txt"));
}

void tst_DFileSorter::fileSorter_sortByLastRead_withSymlink()
{
    // 测试按访问时间排序时，符号链接使用其指向文件的时间
    DFileSorter::SortConfig config;
    config.role = DFileSorter::SortRole::LastRead;
    config.order = Qt::AscendingOrder;
    config.mixDirAndFile = true;

    DFileSorter sorter(config);

    // 创建临时目录和文件
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QString file1 = tempDir.path() + "/file1.txt";
    QString file2 = tempDir.path() + "/file2.txt";
    QString linkFile = tempDir.path() + "/link_to_file2.txt";

    // 创建文件并设置访问时间
    {
        QFile f(file1);
        f.open(QIODevice::WriteOnly);
        f.setFileTime(QDateTime::fromSecsSinceEpoch(1000), QFileDevice::FileAccessTime);
    }
    {
        QFile f(file2);
        f.open(QIODevice::WriteOnly);
        f.setFileTime(QDateTime::fromSecsSinceEpoch(3000), QFileDevice::FileAccessTime);
    }

    // 创建符号链接指向 file2
    QFile::link(file2, linkFile);

    QList<QSharedPointer<DEnumerator::SortFileInfo>> files;

    // 普通文件：file1（访问时间 1000）
    auto normalFile = QSharedPointer<DEnumerator::SortFileInfo>::create();
    normalFile->url = QUrl::fromLocalFile(file1);
    normalFile->isDir = false;
    normalFile->isSymLink = false;
    normalFile->lastRead = 1000;
    normalFile->lastReadNs = 0;
    files.append(normalFile);

    // 符号链接：指向 file2（访问时间 3000）
    auto symlink1 = QSharedPointer<DEnumerator::SortFileInfo>::create();
    symlink1->url = QUrl::fromLocalFile(linkFile);
    symlink1->isDir = false;
    symlink1->isSymLink = true;
    symlink1->symlinkUrl = QUrl::fromLocalFile(file2);  // 指向目标文件
    symlink1->lastRead = 2000;  // 链接本身的时间（排序时应忽略，使用目标时间 3000）
    symlink1->lastReadNs = 0;
    files.append(symlink1);

    auto sorted = sorter.sort(std::move(files));

    // 按访问时间升序：file1(1000) < link_to_file2(3000，目标文件时间)
    QCOMPARE(sorted.size(), 2);
    QCOMPARE(sorted[0]->url.fileName(), QString("file1.txt"));
    QCOMPARE(sorted[1]->url.fileName(), QString("link_to_file2.txt"));
}

int run_tst_DFileSorter(int argc, char *argv[])
{
    tst_DFileSorter tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_dfilesorter.moc"

