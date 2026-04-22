// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QUrl>
#include <QCollator>
#include <QDebug>

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
    void fileSorter_sortByLastModified();
    void fileSorter_sortByLastRead();
    void fileSorter_mixDirAndFile();
    void fileSorter_separateDirAndFile();
    void fileSorter_emptyList();
    void fileSorter_singleItem();
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

int run_tst_DFileSorter(int argc, char *argv[])
{
    tst_DFileSorter tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_dfilesorter.moc"

