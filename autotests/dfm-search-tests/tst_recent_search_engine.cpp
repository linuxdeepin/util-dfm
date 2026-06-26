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

QObject *create_tst_RecentSearchEngine()
{
    return new tst_RecentSearchEngine();
}

#include "tst_recent_search_engine.moc"
