// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDateTime>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTest>
#include <stubext.h>

#include <dfm-search/searchfactory.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/timeresultapi.h>

#include "recentsearch/recentstrategies/recentsearchstrategy.h"

using namespace DFMSEARCH;

namespace {

// 辅助函数：提取所有结果路径
QStringList resultPaths(const SearchResultList &results)
{
    QStringList paths;
    for (const SearchResult &result : results) {
        paths.append(result.path());
    }
    return paths;
}

// 创建模拟测试数据
QList<RecentItem> createTestItems()
{
    const qint64 now = QDateTime::currentDateTime().toSecsSinceEpoch();
    const qint64 yesterday = QDateTime::currentDateTime().addDays(-1).toSecsSinceEpoch();
    const qint64 lastWeek = QDateTime::currentDateTime().addDays(-7).toSecsSinceEpoch();

    return {
        {"/tmp/test/report.pdf", "file:///tmp/test/report.pdf", now},
        {"/tmp/test/meeting-notes.txt", "file:///tmp/test/meeting-notes.txt", yesterday},
        {"/tmp/test/holiday-photo.jpg", "file:///tmp/test/holiday-photo.jpg", now},
        {"/tmp/test/budget.xlsx", "file:///tmp/test/budget.xlsx", lastWeek},
        {"/tmp/test/project-plan.docx", "file:///tmp/test/project-plan.docx", yesterday},
        {"/tmp/test/notes.txt", "file:///tmp/test/notes.txt", yesterday},
    };
}

// 包含黑名单目录下文件的测试数据，用于验证黑名单路径过滤
QList<RecentItem> createBlacklistTestItems()
{
    const qint64 now = QDateTime::currentDateTime().toSecsSinceEpoch();
    const qint64 yesterday = QDateTime::currentDateTime().addDays(-1).toSecsSinceEpoch();

    return {
        // 黑名单目录 /home/uos/Downloads 下的文件（应被过滤）
        {"/home/uos/Downloads/report.pdf", "file:///home/uos/Downloads/report.pdf", now},
        {"/home/uos/Downloads/notes.txt", "file:///home/uos/Downloads/notes.txt", yesterday},
        {"/home/uos/Downloads/sub/deep.xlsx", "file:///home/uos/Downloads/sub/deep.xlsx", now},
        // 名称前缀相同但非黑名单子目录的文件（应保留——路径边界匹配）
        {"/home/uos/Downloads_backup/archive.zip", "file:///home/uos/Downloads_backup/archive.zip", now},
        // 非黑名单目录下的文件（应保留）
        {"/home/uos/Documents/plan.docx", "file:///home/uos/Documents/plan.docx", yesterday},
        {"/tmp/test/notes.txt", "file:///tmp/test/notes.txt", yesterday},
    };
}

} // namespace

// 可测试的 RecentSearchStrategy 子类，提供访问 fetchAddr 的接口
class TestableRecentStrategy : public RecentSearchStrategy
{
public:
    using RecentSearchStrategy::RecentSearchStrategy;

    auto fetchAddr() const {
        return VADDR(TestableRecentStrategy, fetchRecentItems);
    }
};

class tst_RecentSearchEngine : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void search_emptyFetch_returnsEmptyResult();
    void search_noFilter_returnsAllItems();
    void search_keywordFilter_matchesOnlyFilenameWithKeyword();
    void search_extensionFilter_matchesOnlyGivenExtensions();
    void search_timeRangeFilter_keepsOnlyItemsInRange();
    void search_combinedFilter_matchesIntersection();
    void search_dbusFailure_doesNotCrashReturnsEmpty();
    void search_detailedResults_containsExtendedAttributes();
    void search_maxResults_truncatesResultCount();
    void search_resultFoundEnabled_emitsSignalPerItem();
    void search_excludedPathsFilter_removesBlacklistedItems();
    void search_excludedPathsFilter_emptyKeepsAllItems();
    void search_excludedPathsCombinedWithKeyword_appliesBoth();
    void search_excludedPathsFilter_respectsPathBoundary();
};

void tst_RecentSearchEngine::initTestCase()
{
}

void tst_RecentSearchEngine::cleanupTestCase()
{
}

void tst_RecentSearchEngine::search_emptyFetch_returnsEmptyResult()
{
    SearchOptions opts;
    TestableRecentStrategy strategy(opts);

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), []() -> QList<RecentItem> {
        return {};
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("report");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QCOMPARE(results.size(), 0);
}

void tst_RecentSearchEngine::search_noFilter_returnsAllItems()
{
    SearchOptions opts;
    TestableRecentStrategy strategy(opts);
    const auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QCOMPARE(results.size(), testItems.size());
}

void tst_RecentSearchEngine::search_keywordFilter_matchesOnlyFilenameWithKeyword()
{
    SearchOptions opts;
    TestableRecentStrategy strategy(opts);
    const auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("notes");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QStringList paths = resultPaths(results);

    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains("/tmp/test/meeting-notes.txt"));
    QVERIFY(paths.contains("/tmp/test/notes.txt"));
}

void tst_RecentSearchEngine::search_extensionFilter_matchesOnlyGivenExtensions()
{
    SearchOptions opts;
    FileNameOptionsAPI api(opts);
    api.setFileExtensions({"pdf", "txt"});
    TestableRecentStrategy strategy(opts);
    const auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QStringList paths = resultPaths(results);

    QCOMPARE(paths.size(), 3);
    QVERIFY(paths.contains("/tmp/test/report.pdf"));
    QVERIFY(paths.contains("/tmp/test/meeting-notes.txt"));
    QVERIFY(paths.contains("/tmp/test/notes.txt"));
}

void tst_RecentSearchEngine::search_timeRangeFilter_keepsOnlyItemsInRange()
{
    SearchOptions opts;
    TimeRangeFilter filter;
    filter.setTimeField(TimeField::ModifyTime).setYesterday();
    opts.setTimeRangeFilter(filter);
    TestableRecentStrategy strategy(opts);
    const auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();

    // 找出昨天的条目：meeting-notes.txt, project-plan.docx, notes.txt
    QCOMPARE(results.size(), 3);
    QStringList paths = resultPaths(results);
    QVERIFY(paths.contains("/tmp/test/meeting-notes.txt"));
    QVERIFY(paths.contains("/tmp/test/project-plan.docx"));
    QVERIFY(paths.contains("/tmp/test/notes.txt"));
}

void tst_RecentSearchEngine::search_combinedFilter_matchesIntersection()
{
    SearchOptions opts;
    FileNameOptionsAPI api(opts);
    api.setFileExtensions({"txt"});
    TimeRangeFilter filter;
    filter.setTimeField(TimeField::ModifyTime).setYesterday();
    opts.setTimeRangeFilter(filter);
    TestableRecentStrategy strategy(opts);
    const auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("meeting");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    // 关键词 "meeting" + 扩展名 "txt" + 昨天时间范围 交集：仅 meeting-notes.txt
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.first().path(), "/tmp/test/meeting-notes.txt");
}

void tst_RecentSearchEngine::search_dbusFailure_doesNotCrashReturnsEmpty()
{
    SearchOptions opts;
    TestableRecentStrategy strategy(opts);

    stub_ext::StubExt stub;
    // DBus 失败模拟：fetchRecentItems 返回空列表
    stub.set_lamda(strategy.fetchAddr(), []() -> QList<RecentItem> {
        return {};
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    QSignalSpy errorSpy(&strategy, &BaseSearchStrategy::errorOccurred);
    SearchQuery query = SearchFactory::createQuery("any keyword");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QCOMPARE(results.size(), 0);
    // 不应该发射 error 信号（DBus 失败本身不标记为搜索错误，只返回空结果）
    QCOMPARE(errorSpy.count(), 0);
}

void tst_RecentSearchEngine::search_detailedResults_containsExtendedAttributes()
{
    SearchOptions opts;
    opts.setDetailedResultsEnabled(true);
    TestableRecentStrategy strategy(opts);
    auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("report");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QCOMPARE(results.size(), 1);

    SearchResult result = results.first();
    FileNameResultAPI api(result);
    QCOMPARE(result.path(), "/tmp/test/report.pdf");
    QCOMPARE(api.filename(), QString("report.pdf"));
    QCOMPARE(api.fileExtension(), QString("pdf"));
    QCOMPARE(api.isDirectory(), false);
    QCOMPARE(api.isHidden(), false);
    QCOMPARE(api.modifyTimestamp(), testItems[0].modified);
}

void tst_RecentSearchEngine::search_maxResults_truncatesResultCount()
{
    SearchOptions opts;
    opts.setMaxResults(2);
    TestableRecentStrategy strategy(opts);
    const auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QCOMPARE(results.size(), 2); // 总共有 6 条，只返回前 2 条
}

void tst_RecentSearchEngine::search_resultFoundEnabled_emitsSignalPerItem()
{
    SearchOptions opts;
    opts.setResultFoundEnabled(true);
    opts.setMaxResults(3);
    TestableRecentStrategy strategy(opts);
    const auto testItems = createTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy foundSpy(&strategy, &BaseSearchStrategy::resultFound);
    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(foundSpy.count(), 3); // maxResults = 3，所以发射 3 次
    QCOMPARE(finishedSpy.count(), 1);
}

void tst_RecentSearchEngine::search_excludedPathsFilter_removesBlacklistedItems()
{
    SearchOptions opts;
    opts.setSearchExcludedPaths({"/home/uos/Downloads"});
    TestableRecentStrategy strategy(opts);
    const auto testItems = createBlacklistTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QStringList paths = resultPaths(results);

    // 黑名单目录 /home/uos/Downloads 下的 3 个文件被过滤；
    // /home/uos/Downloads_backup/archive.zip 因路径边界匹配而保留，共 3 个
    QCOMPARE(paths.size(), 3);
    QVERIFY(paths.contains("/home/uos/Documents/plan.docx"));
    QVERIFY(paths.contains("/tmp/test/notes.txt"));
    QVERIFY(paths.contains("/home/uos/Downloads_backup/archive.zip"));
    QVERIFY(!paths.contains("/home/uos/Downloads/report.pdf"));
    QVERIFY(!paths.contains("/home/uos/Downloads/notes.txt"));
    QVERIFY(!paths.contains("/home/uos/Downloads/sub/deep.xlsx"));
}

void tst_RecentSearchEngine::search_excludedPathsFilter_emptyKeepsAllItems()
{
    SearchOptions opts;
    // 不设置黑名单路径
    TestableRecentStrategy strategy(opts);
    const auto testItems = createBlacklistTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    // 无黑名单时全部保留
    QCOMPARE(results.size(), testItems.size());
}

void tst_RecentSearchEngine::search_excludedPathsCombinedWithKeyword_appliesBoth()
{
    SearchOptions opts;
    opts.setSearchExcludedPaths({"/home/uos/Downloads"});
    TestableRecentStrategy strategy(opts);
    const auto testItems = createBlacklistTestItems();

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [testItems]() -> QList<RecentItem> {
        return testItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    // 关键词 "notes" 命中 /home/uos/Downloads/notes.txt（被黑名单过滤）与 /tmp/test/notes.txt（保留）
    SearchQuery query = SearchFactory::createQuery("notes");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QStringList paths = resultPaths(results);

    QCOMPARE(paths.size(), 1);
    QVERIFY(paths.contains("/tmp/test/notes.txt"));
    QVERIFY(!paths.contains("/home/uos/Downloads/notes.txt"));
}

void tst_RecentSearchEngine::search_excludedPathsFilter_respectsPathBoundary()
{
    // 验证路径边界匹配：黑名单 /home/uos/Downloads 不应误匹配 /home/uos/Downloads_backup
    SearchOptions opts;
    opts.setSearchExcludedPaths({"/home/uos/Downloads"});
    TestableRecentStrategy strategy(opts);

    const qint64 now = QDateTime::currentDateTime().toSecsSinceEpoch();
    const QList<RecentItem> boundaryItems = {
        // 黑名单目录下的文件（应过滤）
        {"/home/uos/Downloads/file.txt", "file:///home/uos/Downloads/file.txt", now},
        // 名称前缀相同但非子目录（应保留）
        {"/home/uos/Downloads_backup/file.txt", "file:///home/uos/Downloads_backup/file.txt", now},
        {"/home/uos/Downloads_archive/file.txt", "file:///home/uos/Downloads_archive/file.txt", now},
    };

    stub_ext::StubExt stub;
    stub.set_lamda(strategy.fetchAddr(), [boundaryItems]() -> QList<RecentItem> {
        return boundaryItems;
    });

    QSignalSpy finishedSpy(&strategy, &BaseSearchStrategy::searchFinished);
    SearchQuery query = SearchFactory::createQuery("");
    strategy.search(query);

    QCOMPARE(finishedSpy.count(), 1);
    SearchResultList results = finishedSpy.takeFirst().at(0).value<SearchResultList>();
    QStringList paths = resultPaths(results);

    // 仅 /home/uos/Downloads/file.txt 被过滤，_backup 和 _archive 因路径边界而保留
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains("/home/uos/Downloads_backup/file.txt"));
    QVERIFY(paths.contains("/home/uos/Downloads_archive/file.txt"));
    QVERIFY(!paths.contains("/home/uos/Downloads/file.txt"));
}

QObject *create_tst_RecentSearchEngine()
{
    return new tst_RecentSearchEngine();
}

#include "tst_recent_search_engine.moc"
