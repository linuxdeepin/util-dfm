// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>

#include <dfm-search/dsearch_global.h>
#include <dfm-search-lib/utils/filenameblacklistmatcher.h>

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
    void testFileNameBlacklistMatcher();

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

    // 结果验证
    QVERIFY2(status.has_value(), "Could not retrieve status (file missing/permission error?)");

    // 定义有效状态列表（小写）
    static const QSet<QString> validStatuses {
        "loading",
        "scanning",
        "monitoring",
        "closed"
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

QObject *create_tst_SearchUtils()
{
    return new tst_SearchUtils();
}

#include "tst_search_utils.moc"
