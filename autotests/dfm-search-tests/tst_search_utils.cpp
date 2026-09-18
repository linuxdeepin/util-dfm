// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/PhraseQuery.h>
#include <lucene++/TermQuery.h>

#include <dfm-search/dsearch_global.h>
#include <dfm-search-lib/utils/filenameblacklistmatcher.h>
#include <dfm-search-lib/utils/lucenequeryutils.h>
#include <dfm-search-lib/utils/searchutility.h>

using namespace DFMSEARCH;

class tst_SearchUtils : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testGlobal();
    void testPinyin();
    void testPinyinAcronym();
    void testAnythingStatus();
    void testFileNameIndexStatusMapping();
    void testFileNameBlacklistMatcher();
    void testNGramSearchQuery();

private:
    void doTestPinyin(const QString &caseName, const QString &input, bool expected);
    void doTestPinyinAcronym(const QString &caseName, const QString &input, bool expected);
    void doFileNameBlacklistMatchTest(const QString &caseName,
                                     const QString &inputPath,
                                     const QStringList &blacklistEntries,
                                     bool expected);
};

void tst_SearchUtils::initTestCase()
{
    // Setup that runs once before all tests
}

void tst_SearchUtils::cleanupTestCase()
{
    // Cleanup that runs once after all tests
}

void tst_SearchUtils::doTestPinyin(const QString &caseName, const QString &input, bool expected)
{
    bool actual = Global::isPinyinSequence(input);
    QCOMPARE(actual, expected);
}

void tst_SearchUtils::testPinyin()
{
    // 有效拼音测试集
    QList<QPair<QString, bool>> validCases = {
        // 单韵母
        { "a", true },
        { "o", true },
        { "e", true },
        { "O", true },

        // 声母+韵母
        { "ba", true },
        { "po", true },
        { "mi", true },

        // 特殊音节
        { "zhi", true },
        { "chi", true },
        { "shi", true },

        // 复韵母
        { "ai", true },
        { "er", true },
        { "ang", true },

        // ü相关
        { "lv", true },
        { "lüe", true },

        // 多音节
        { "nihao", true },
        { "pinyin", true },
        { "zhongwen", true },
        { "shuang", true },
        { "xian", true },
        { "quan", true },
        { "jiang", true },

        // 大小写混合
        { "ZhongGuo", true },
        { "XIONG", true },
        { "PinYin", true },

        { "make", true },
        { "xinjian", true },
        { "zhangsheng", true },
        { "wendan", true },
        { "wendang", true },
        { "xiaa", true },
        { "chaojichangdeyijuhua", true },
        { "chengong", true },
        { "shibai", true },
        { "case", true },
        { "sougou", true },
        { "sousuo", true },
        { "jieshi", true },
        { "zongjie", true },
        { "jiu", true },
        { "chengdu", true },
        { "beijing", true },
        { "xian", true },
        { "chongqing", true },
        { "chongqin", true },
        { "chenqin", true },
        { "shanghai", true },
    };

    // 无效拼音测试集
    QList<QPair<QString, bool>> invalidCases = {
        // 基本无效情况
        { "", false },
        { "vvv", false },
        { "kkkk", false },
        { "i", false },
        { "u", false },

        // 非法拼音组合
        { "xqiong", false },

        // 英文单词
        { "hello", false },
        { "world", false },
        { "cmake", false },

        // 数字和特殊字符
        { "zh@ng", false },

        // 不完整或错误的拼音
        { "zh", false },
        { "zho", false },
        { "jx", false },

        // 特殊规则测试
        { "yi", true },
        { "wu", true },
        { "yu", true },
        { "yue", true },
        { "yuan", true },

        // 边界情况
        { "v", false },
        { "ü", false },
        { "ng", false },
        { "gn", false },

        { "123", false },
        { "z", false },
        { "zh", false },
        { "p", false },
        { "m", false },
        { "b", false },
        { "jiv", false },
    };

    for (const auto &pair : validCases) {
        doTestPinyin("有效拼音验证", pair.first, pair.second);
    }

    for (const auto &pair : invalidCases) {
        doTestPinyin("无效拼音检测", pair.first, pair.second);
    }
}

void tst_SearchUtils::doTestPinyinAcronym(const QString &caseName, const QString &input, bool expected)
{
    bool actual = Global::isPinyinAcronymSequence(input);
    QCOMPARE(actual, expected);
}

void tst_SearchUtils::testPinyinAcronym()
{
    // 有效拼音首字母测试集
    QList<QPair<QString, bool>> validCases = {
        // 基本有效情况
        { "n", true },
        { "nh", true },
        { "wd", true },
        { "xj", true },
        { "zhzw", true },
        { "dfm", true },
        { "ABC", true },
        { "AbC", true },
        { "hello", true },
        { "a", true },
        { "z", true },
        // 包含数字和符号的有效情况
        { "nh123", true },
        { "wd_v1", true },
        { "test-file", true },
        { "config.bak", true },
        { "a1b2c3", true },
        { "file_2023", true },
    };

    // 无效拼音首字母测试集
    QList<QPair<QString, bool>> invalidCases = {
        // 基本无效情况
        { "", false },
        { "你好", false },
        { "n好", false },
        { "123", false },
        { "._-", false },
    };

    for (const auto &pair : validCases) {
        doTestPinyinAcronym("有效首字母验证", pair.first, pair.second);
    }

    for (const auto &pair : invalidCases) {
        doTestPinyinAcronym("无效首字母检测", pair.first, pair.second);
    }
}

void tst_SearchUtils::testAnythingStatus()
{
    qDebug() << "=== Starting Anything Status Test ===";

    // 测试状态获取
    auto status = Global::fileNameIndexStatus();

    if (!status.has_value()) {
        QWARN("filename index status unavailable (no local index) - skipping runtime check");
        return;
    }

    // 新状态映射返回 "scanning"/"updating"/"monitoring"（"loading"/"closed" 不再出现）
    static const QSet<QString> validStatuses {
        "scanning",
        "updating",
        "monitoring"
    };

    // 状态有效性检查
    const QString currentStatus = status.value();
    QVERIFY2(validStatuses.contains(currentStatus),
             QString("Invalid anything status value: %1\nExpected one of: %2")
                 .arg(currentStatus, validStatuses.values().join(", "))
                 .toUtf8()
                 .constData());

    // 成功输出
    qInfo() << "Test Passed. Current anything status:" << currentStatus;
}

void tst_SearchUtils::testFileNameIndexStatusMapping()
{
    struct Case {
        QString name;
        QByteArray json;
        std::optional<QString> expectedStatus;
        bool expectedReady;
    };

    const QList<Case> cases = {
        // 首次创建中（Create 启动时 removeIndexStatusFile，最小字段）
        { "create-in-progress",
          R"({"createInProgress": true})",
          QStringLiteral("scanning"), false },
        // 中断后继续创建（版本重建，无 lastUpdateTime）
        { "create-resumed",
          R"({"createInProgress": true, "version": 1})",
          QStringLiteral("scanning"), false },
        // dirty 重启后的恢复 Update 中（索引滞后不可信 → 降级）
        { "recovery-update",
          R"({"state": "dirty", "createInProgress": false, "updateInProgress": true, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("scanning"), false },
        // 运行中 needsRebuild 触发的 rebuild Update 中（新路径文件未索引 → 降级）
        { "rebuild-update",
          R"({"state": "dirty", "updateInProgress": true, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("scanning"), false },
        // Update 失败/中断后标志保持（持续降级直到恢复成功）
        { "update-failed-persisted",
          R"({"state": "dirty", "updateInProgress": true, "needsRebuild": true, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("scanning"), false },
        // 大规模事件积压处理中（backlogExceeded → "updating" → 降级 Realtime）
        { "backlog-exceeded",
          R"({"state": "dirty", "backlogExceeded": true, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("updating"), false },
        // 崩溃时正在积压（dirty + backlog 残留，启动时保持 → 持续降级直到恢复完成）
        { "backlog-crash-residue",
          R"({"state": "dirty", "backlogExceeded": true, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("updating"), false },
        // 积压处理完成（backlogExceeded 清除 → 恢复索引搜索）
        { "backlog-resolved",
          R"({"state": "clean", "backlogExceeded": false, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("monitoring"), true },
        // 普通事件增量（dirty 但无恢复/积压标志，索引可用）
        { "incremental-dirty",
          R"({"state": "dirty", "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("monitoring"), true },
        // 恢复/重建/积压处理完成
        { "clean",
          R"({"state": "clean", "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("monitoring"), true },
        // 从未成功完成过任务（无 lastUpdateTime、无进行中标志）
        { "never-completed",
          R"({"state": "dirty"})",
          QStringLiteral("scanning"), false },
        // 双标志同时置位（createInProgress 优先级最高）
        { "both-flags",
          R"({"createInProgress": true, "updateInProgress": true, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("scanning"), false },
        // backlog + update 同时置位（updateInProgress 优先级高于 backlogExceeded）
        { "update-plus-backlog",
          R"({"updateInProgress": true, "backlogExceeded": true, "lastUpdateTime": "2026-09-18T10:00:00"})",
          QStringLiteral("scanning"), false },
        // JSON 无效
        { "invalid-json",
          "not-json",
          std::nullopt, false },
        // 空内容
        { "empty-content",
          "",
          std::nullopt, false },
    };

    for (const Case &c : cases) {
        const auto actualStatus = Global::fileNameIndexStatusFromJson(c.json);
        QVERIFY2(actualStatus == c.expectedStatus,
                 qPrintable(QString("case '%1': status mismatch, got %2")
                                .arg(c.name,
                                     actualStatus.has_value() ? actualStatus.value() : QStringLiteral("nullopt"))));
        QVERIFY2(Global::fileNameIndexReadyForSearchFromJson(c.json) == c.expectedReady,
                 qPrintable(QString("case '%1': ready mismatch").arg(c.name)));
    }
}

void tst_SearchUtils::doFileNameBlacklistMatchTest(const QString &caseName,
                                                   const QString &inputPath,
                                                   const QStringList &blacklistEntries,
                                                   bool expected)
{
    const bool actual = Global::BlacklistMatcher::isPathBlacklisted(inputPath, blacklistEntries);
    QCOMPARE(actual, expected);
}

void tst_SearchUtils::testFileNameBlacklistMatcher()
{
    doFileNameBlacklistMatchTest("绝对路径-自身命中",
                                 "/home/test/workspace",
                                 { "/home/test/workspace" },
                                 true);

    doFileNameBlacklistMatchTest("绝对路径-子路径命中",
                                 "/home/test/workspace/a.txt",
                                 { "/home/test/workspace" },
                                 true);

    doFileNameBlacklistMatchTest("绝对路径-边界不误匹配",
                                 "/home/test/workspace2",
                                 { "/home/test/workspace" },
                                 false);

    doFileNameBlacklistMatchTest("目录名-直接命中",
                                 "/home/test/workspace",
                                 { "workspace" },
                                 true);

    doFileNameBlacklistMatchTest("目录名-深层命中",
                                 "/home/test/aa/bb/workspace",
                                 { "workspace" },
                                 true);

    doFileNameBlacklistMatchTest("目录名-不命中相似名称",
                                 "/home/test/aa/bb/myworkspace",
                                 { "workspace" },
                                 false);
}

void tst_SearchUtils::testGlobal()
{
    // Test supported content search extensions
    QStringList testExtensions = { "txt", "pdf", "docx", "unknown" };
    for (const auto &ext : testExtensions) {
        bool result = Global::isSupportedContentSearchExtension(ext);
        QString message = QString("Check if '%1' is supported").arg(ext);
        QVERIFY2(result == (ext != "unknown"),
                 message.toUtf8().constData());
    }

    // Test default content search extensions
    QStringList defaultExtensions = Global::defaultContentSearchExtensions();
    QVERIFY2(!defaultExtensions.isEmpty(), "Default supported content search extensions should not be empty");

    // Test content index directory
    QVERIFY2(!Global::contentIndexDirectory().isEmpty(), "Content index directory should not be empty");

    // Test path in content index directory
    QString testPath = QDir::homePath() + "/test.txt";
    Global::isPathInContentIndexDirectory(testPath);

    // Test filename index directory
    QVERIFY2(!Global::fileNameIndexDirectory().isEmpty(), "Filename index directory should not be empty");

    // Test default indexed dirs
    const auto &dirs = Global::defaultIndexedDirectory();
    QVERIFY2(!dirs.isEmpty(), "Default indexed directories should not be empty");

    // Test default blacklist paths
    const auto &blacklistPaths = Global::defaultBlacklistPaths();
    Q_UNUSED(blacklistPaths);
}

void tst_SearchUtils::testNGramSearchQuery()
{
    Lucene::QueryPtr oneCharQuery = LuceneQueryUtils::buildNGramSearchQuery("contents", "A");
    Lucene::TermQueryPtr oneCharTermQuery = boost::dynamic_pointer_cast<Lucene::TermQuery>(oneCharQuery);
    QVERIFY(oneCharTermQuery);
    QCOMPARE(oneCharTermQuery->getTerm()->field(), Lucene::String(L"contents"));
    QCOMPARE(oneCharTermQuery->getTerm()->text(), Lucene::String(L"a"));

    Lucene::QueryPtr twoCharQuery = LuceneQueryUtils::buildNGramSearchQuery("contents", "Ab");
    Lucene::TermQueryPtr twoCharTermQuery = boost::dynamic_pointer_cast<Lucene::TermQuery>(twoCharQuery);
    QVERIFY(twoCharTermQuery);
    QCOMPARE(twoCharTermQuery->getTerm()->text(), Lucene::String(L"ab"));

    Lucene::QueryPtr evenQuery = LuceneQueryUtils::buildNGramSearchQuery("contents", "abcdef");
    Lucene::PhraseQueryPtr evenPhraseQuery = boost::dynamic_pointer_cast<Lucene::PhraseQuery>(evenQuery);
    QVERIFY(evenPhraseQuery);
    QCOMPARE(evenPhraseQuery->getTerms().size(), 3);
    QCOMPARE(evenPhraseQuery->getTerms()[0]->text(), Lucene::String(L"ab"));
    QCOMPARE(evenPhraseQuery->getTerms()[1]->text(), Lucene::String(L"cd"));
    QCOMPARE(evenPhraseQuery->getTerms()[2]->text(), Lucene::String(L"ef"));
    QCOMPARE(evenPhraseQuery->getPositions()[0], 1);
    QCOMPARE(evenPhraseQuery->getPositions()[1], 5);
    QCOMPARE(evenPhraseQuery->getPositions()[2], 9);

    Lucene::QueryPtr oddQuery = LuceneQueryUtils::buildNGramSearchQuery("contents", "abcde");
    Lucene::PhraseQueryPtr oddPhraseQuery = boost::dynamic_pointer_cast<Lucene::PhraseQuery>(oddQuery);
    QVERIFY(oddPhraseQuery);
    QCOMPARE(oddPhraseQuery->getTerms().size(), 3);
    QCOMPARE(oddPhraseQuery->getTerms()[0]->text(), Lucene::String(L"ab"));
    QCOMPARE(oddPhraseQuery->getTerms()[1]->text(), Lucene::String(L"cd"));
    QCOMPARE(oddPhraseQuery->getTerms()[2]->text(), Lucene::String(L"de"));
    QCOMPARE(oddPhraseQuery->getPositions()[0], 1);
    QCOMPARE(oddPhraseQuery->getPositions()[1], 5);
    QCOMPARE(oddPhraseQuery->getPositions()[2], 7);
}

QObject *create_tst_SearchUtils()
{
    return new tst_SearchUtils();
}

#include "tst_search_utils.moc"
