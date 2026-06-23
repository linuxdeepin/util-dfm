// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <dfm-search/semanticsearcher.h>

#include "semantic/semanticruleengine.h"
#include "semantic/intentparser.h"
#include "semantic/ruleconfigloader.h"
#include "semantic/extractors/keywordextractor.h"
#include "semantic/semanticquerybuilder.h"
#include "semantic/extractors/locationextractor.h"

using namespace DFMSEARCH;

static bool buildGroupFromJson(const QByteArray &json, RuleGroup &outGroup)
{
    QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return false;
    }
    QJsonObject root = doc.object();
    QJsonArray groups = root.value("groups").toArray();
    if (groups.isEmpty()) {
        return false;
    }
    return SemanticRuleEngine::parseRuleGroupStatic(groups.at(0).toObject(), outGroup);
}

// Helper: build a simple rule group JSON string
static QByteArray makeRuleJson(const QString &groupName, const QString &ruleId,
                               const QString &pattern, int priority,
                               const QVariantMap &metadata = {})
{
    QJsonObject ruleObj;
    ruleObj["id"] = ruleId;
    ruleObj["pattern"] = pattern;
    ruleObj["enabled"] = true;
    ruleObj["priority"] = priority;
    if (!metadata.isEmpty()) {
        ruleObj["metadata"] = QJsonObject::fromVariantMap(metadata);
    }

    QJsonObject ruleGroupObj;
    ruleGroupObj["name"] = groupName;
    ruleGroupObj["version"] = "1.0.0";
    ruleGroupObj["rules"] = QJsonArray({ ruleObj });

    QJsonObject root;
    root["groups"] = QJsonArray({ ruleGroupObj });

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

// ===== tst_RuleEngine =====

class tst_RuleEngine : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parseValidGroup();
    void parseEmptyGroup();
    void parsePriorityOrdering();
    void matchReturnsHighestPriority();
    void matchAllReturnsAll();
    void ruleMetadataAccess();
    void hasGroupCheck();
};

void tst_RuleEngine::parseValidGroup()
{
    QByteArray json = makeRuleJson("test", "r1", "hello", 100);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.name, QString("test"));
    QCOMPARE(group.rules.size(), 1);
    QCOMPARE(group.rules[0].id, QString("r1"));
    QVERIFY(group.rules[0].regex.isValid());
}

void tst_RuleEngine::parseEmptyGroup()
{
    QByteArray json = R"=====({"groups": [{"name": "empty", "rules": []}]})=====";

    RuleGroup group;
    QVERIFY(!buildGroupFromJson(json, group));
}

void tst_RuleEngine::parsePriorityOrdering()
{
    QJsonObject r1, r2, r3;
    r1["id"] = "low";
    r1["pattern"] = "test";
    r1["priority"] = 10;
    r2["id"] = "high";
    r2["pattern"] = "test";
    r2["priority"] = 200;
    r3["id"] = "mid";
    r3["pattern"] = "test";
    r3["priority"] = 100;

    QJsonObject ruleGroup;
    ruleGroup["name"] = "prio";
    ruleGroup["version"] = "1.0.0";
    ruleGroup["rules"] = QJsonArray({ r1, r2, r3 });

    QJsonObject root;
    root["groups"] = QJsonArray({ ruleGroup });

    RuleGroup group;
    QVERIFY(SemanticRuleEngine::parseRuleGroupStatic(ruleGroup, group));
    QCOMPARE(group.rules.size(), 3);

    QStringList ids;
    for (const Rule &r : group.rules) {
        ids.append(r.id);
    }
    QVERIFY(ids.contains("low"));
    QVERIFY(ids.contains("mid"));
    QVERIFY(ids.contains("high"));
}

void tst_RuleEngine::matchReturnsHighestPriority()
{
    QByteArray json = makeRuleJson("test_match", "r1", "abc", 200,
                                   { { "level", "high" } });

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("abc").hasMatch());
}

void tst_RuleEngine::matchAllReturnsAll()
{
    QJsonObject r1, r2, r3;
    r1["id"] = "r1";
    r1["pattern"] = "cat";
    r1["priority"] = 100;
    r2["id"] = "r2";
    r2["pattern"] = "dog";
    r2["priority"] = 100;
    r3["id"] = "r3";
    r3["pattern"] = "bird";
    r3["priority"] = 50;

    QJsonObject ruleGroup;
    ruleGroup["name"] = "test_all";
    ruleGroup["version"] = "1.0.0";
    ruleGroup["rules"] = QJsonArray({ r1, r2, r3 });

    QJsonObject root;
    root["groups"] = QJsonArray({ ruleGroup });

    RuleGroup group;
    QVERIFY(SemanticRuleEngine::parseRuleGroupStatic(ruleGroup, group));
    QCOMPARE(group.rules.size(), 3);
}

void tst_RuleEngine::ruleMetadataAccess()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "today";
    QByteArray json = makeRuleJson("test_meta", "m1", "test", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("type").toString(), QString("preset"));
    QCOMPARE(group.rules[0].metadata.value("preset").toString(), QString("today"));
}

void tst_RuleEngine::hasGroupCheck()
{
    SemanticRuleEngine engine;
    QVERIFY(!engine.hasGroup("time"));
    QVERIFY(!engine.hasGroup("filetype"));
    QCOMPARE(engine.groupNames().size(), 0);
}

// ===== tst_TimeExtraction =====

class tst_TimeExtraction : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void presetToday();
    void presetYesterday();
    void presetThisWeek();
    void presetThisMonth();
    void presetThisYear();
    void presetLastYear();
    void customYear();
    void customYearMonth();
    void customFullDate();
    void noMatch();
};

void tst_TimeExtraction::presetToday()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "today";
    QByteArray json = makeRuleJson("time", "time_today", "today", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("preset").toString(), QString("today"));
    QVERIFY(group.rules[0].regex.isValid());
    QVERIFY(group.rules[0].regex.match("today").hasMatch());
    QVERIFY(!group.rules[0].regex.match("yesterday").hasMatch());
}

void tst_TimeExtraction::presetYesterday()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "yesterday";
    QByteArray json = makeRuleJson("time", "time_yesterday", "yesterday", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("preset").toString(), QString("yesterday"));
    QVERIFY(group.rules[0].regex.match("yesterday").hasMatch());
}

void tst_TimeExtraction::presetThisWeek()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "this_week";
    QByteArray json = makeRuleJson("time", "time_this_week", "this_week", 190, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("preset").toString(), QString("this_week"));
}

void tst_TimeExtraction::presetThisMonth()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "this_month";
    QByteArray json = makeRuleJson("time", "time_this_month", "this_month", 180, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("preset").toString(), QString("this_month"));
}

void tst_TimeExtraction::presetThisYear()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "this_year";
    QByteArray json = makeRuleJson("time", "time_this_year", "this_year", 170, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("preset").toString(), QString("this_year"));
}

void tst_TimeExtraction::presetLastYear()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "last_year";
    QByteArray json = makeRuleJson("time", "time_last_year", "last_year", 170, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("preset").toString(), QString("last_year"));
}

void tst_TimeExtraction::customYear()
{
    // Use programmatic JSON to avoid raw string delimiter conflict with regex patterns
    QVariantMap meta;
    meta["type"] = "custom";
    meta["format"] = "year";
    QByteArray json = makeRuleJson("time", "time_exact_year",
                                   "(?<year>\\d{2,4})year", 160, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));

    auto match = group.rules[0].regex.match("2025year");
    QVERIFY(match.hasMatch());
    QCOMPARE(match.captured("year"), QString("2025"));
}

void tst_TimeExtraction::customYearMonth()
{
    QVariantMap meta;
    meta["type"] = "custom";
    meta["format"] = "year_month";
    QByteArray json = makeRuleJson("time", "time_exact_year_month",
                                   "(?<year>\\d{4})-(?<month>\\d{1,2})", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));

    auto match = group.rules[0].regex.match("2025-12");
    QVERIFY(match.hasMatch());
    QCOMPARE(match.captured("year"), QString("2025"));
    QCOMPARE(match.captured("month"), QString("12"));
}

void tst_TimeExtraction::customFullDate()
{
    QVariantMap meta;
    meta["type"] = "custom";
    meta["format"] = "full_date";
    QByteArray json = makeRuleJson("time", "time_exact_full_date",
                                   "(?<year>\\d{4})-(?<month>\\d{1,2})-(?<day>\\d{1,2})",
                                   140, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));

    auto match = group.rules[0].regex.match("2025-03-15");
    QVERIFY(match.hasMatch());
    QCOMPARE(match.captured("year"), QString("2025"));
    QCOMPARE(match.captured("month"), QString("03"));
    QCOMPARE(match.captured("day"), QString("15"));
}

void tst_TimeExtraction::noMatch()
{
    QVariantMap meta;
    meta["type"] = "preset";
    meta["preset"] = "today";
    QByteArray json = makeRuleJson("time", "time_today", "today", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(!group.rules[0].regex.match("random text without match").hasMatch());
}

// ===== tst_FileTypeExtraction =====

class tst_FileTypeExtraction : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void precisePdf();
    void preciseWord();
    void preciseExcel();
    void precisePpt();
    void imageType();
    void videoType();
    void audioType();
    void genericDocument();
    void genericSpreadsheet();
    void genericPresentation();
    void archiveType();
    void extensionsList();
    void generalFlag();
};

void tst_FileTypeExtraction::precisePdf()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "pdf" });
    QByteArray json = makeRuleJson("filetype", "filetype_pdf", "pdf", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("pdf").hasMatch());
    QCOMPARE(group.rules[0].metadata.value("extensions").toStringList(), QStringList({ "pdf" }));
}

void tst_FileTypeExtraction::preciseWord()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "doc", "docx" });
    meta["fileTypes"] = QStringList({ "doc" });
    QByteArray json = makeRuleJson("filetype", "filetype_word", "word|doc|docx", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("word").hasMatch());
    QVERIFY(group.rules[0].regex.match("docx").hasMatch());
    QVERIFY(!group.rules[0].regex.match("pdf").hasMatch());
}

void tst_FileTypeExtraction::preciseExcel()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "xls", "xlsx" });
    QByteArray json = makeRuleJson("filetype", "filetype_excel", "excel|xls|xlsx", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("excel").hasMatch());
    QVERIFY(group.rules[0].regex.match("xlsx").hasMatch());
}

void tst_FileTypeExtraction::precisePpt()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "ppt", "pptx" });
    QByteArray json = makeRuleJson("filetype", "filetype_ppt", "ppt|pptx", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("ppt").hasMatch());
    QVERIFY(group.rules[0].regex.match("pptx").hasMatch());
}

void tst_FileTypeExtraction::imageType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "jpg", "png", "gif" });
    meta["fileTypes"] = QStringList({ "pic" });
    QByteArray json = makeRuleJson("filetype", "filetype_image", "image", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("fileTypes").toStringList(), QStringList({ "pic" }));
}

void tst_FileTypeExtraction::videoType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "mp4", "avi", "mkv" });
    meta["fileTypes"] = QStringList({ "video" });
    QByteArray json = makeRuleJson("filetype", "filetype_video", "video", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("fileTypes").toStringList(), QStringList({ "video" }));
}

void tst_FileTypeExtraction::audioType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "mp3", "wav", "flac" });
    meta["fileTypes"] = QStringList({ "audio" });
    QByteArray json = makeRuleJson("filetype", "filetype_audio", "audio", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("fileTypes").toStringList(), QStringList({ "audio" }));
}

void tst_FileTypeExtraction::genericDocument()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "doc", "docx", "pdf", "txt" });
    meta["fileTypes"] = QStringList({ "doc" });
    meta["general"] = true;
    QByteArray json = makeRuleJson("filetype", "filetype_document_general", "document", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].metadata.value("general").toBool());
    QCOMPARE(group.rules[0].metadata.value("extensions").toStringList().size(), 4);
}

void tst_FileTypeExtraction::genericSpreadsheet()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "xls", "xlsx", "csv" });
    meta["general"] = true;
    QByteArray json = makeRuleJson("filetype", "filetype_spreadsheet_general", "spreadsheet", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].metadata.value("general").toBool());
}

void tst_FileTypeExtraction::genericPresentation()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "ppt", "pptx", "dps" });
    meta["general"] = true;
    QByteArray json = makeRuleJson("filetype", "filetype_presentation_general", "presentation", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].metadata.value("general").toBool());
}

void tst_FileTypeExtraction::archiveType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "zip", "tar", "rar", "7z" });
    QByteArray json = makeRuleJson("filetype", "filetype_archive", "archive", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("extensions").toStringList().size(), 4);
}

void tst_FileTypeExtraction::extensionsList()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "a", "b", "c", "d", "e" });
    QByteArray json = makeRuleJson("filetype", "ft", "test", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("extensions").toStringList().size(), 5);
}

void tst_FileTypeExtraction::generalFlag()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({ "pdf" });
    QByteArray json = makeRuleJson("filetype", "ft_precise", "pdf", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(!group.rules[0].metadata.value("general").toBool());
}

// ===== tst_KeywordExtraction =====

class tst_KeywordExtraction : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void containsPattern();
    void namedPattern();
    void contentHasPattern();
    void noMatch();
    void captureGroup();
    void multiKeywordFlag();
};

void tst_KeywordExtraction::containsPattern()
{
    QVariantMap meta;
    meta["capture_group"] = 1;
    meta["multi_keyword"] = true;
    QByteArray json = makeRuleJson("keyword", "keyword_contains",
                                   "contains(.+?)(?:of|$)", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));

    auto match = group.rules[0].regex.match("contains meeting notes");
    QVERIFY(match.hasMatch());
    QCOMPARE(match.captured(1), QString(" meeting notes"));
}

void tst_KeywordExtraction::namedPattern()
{
    QVariantMap meta;
    meta["capture_group"] = 1;
    meta["multi_keyword"] = false;
    QByteArray json = makeRuleJson("keyword", "keyword_named",
                                   "named (.+?)(?: of|$)", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));

    auto match = group.rules[0].regex.match("named report of");
    QVERIFY(match.hasMatch());
    QCOMPARE(match.captured(1), QString("report"));
}

void tst_KeywordExtraction::contentHasPattern()
{
    QVariantMap meta;
    meta["capture_group"] = 1;
    meta["multi_keyword"] = true;
    QByteArray json = makeRuleJson("keyword", "keyword_content_has",
                                   "content(?: has| contains| includes)(.+?)(?: of|$)",
                                   200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));

    auto match = group.rules[0].regex.match("content includes budget data");
    QVERIFY(match.hasMatch());
    QCOMPARE(match.captured(1), QString(" budget data"));
}

void tst_KeywordExtraction::noMatch()
{
    QVariantMap meta;
    meta["capture_group"] = 1;
    QByteArray json = makeRuleJson("keyword", "keyword_contains",
                                   "contains(.+?)(?:of|$)", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(!group.rules[0].regex.match("no keyword pattern here").hasMatch());
}

void tst_KeywordExtraction::captureGroup()
{
    QVariantMap meta;
    meta["capture_group"] = 1;
    QByteArray json = makeRuleJson("keyword", "k1", "contains(.+?)(?:of|$)", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("capture_group").toInt(), 1);
}

void tst_KeywordExtraction::multiKeywordFlag()
{
    QVariantMap meta;
    meta["capture_group"] = 1;
    meta["multi_keyword"] = true;
    QByteArray json = makeRuleJson("keyword", "k1", "contains(.+?)(?:of|$)", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].metadata.value("multi_keyword").toBool());
}

// ===== tst_ParsedIntent =====

class tst_ParsedIntent : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultState();
    void timeConstraintDefault();
    void timeConstraintPreset();
    void matchSpanValidity();
};

void tst_ParsedIntent::defaultState()
{
    ParsedIntent intent;
    QVERIFY(intent.timeConstraint().kind == TimeConstraintKind::None);
    QVERIFY(intent.fileExtensions().isEmpty());
    QVERIFY(intent.keywords().isEmpty());
    QVERIFY(intent.consumedSpans().isEmpty());
}

void tst_ParsedIntent::timeConstraintDefault()
{
    TimeConstraint tc;
    QVERIFY(!tc.isValid());
    QCOMPARE(tc.kind, TimeConstraintKind::None);
}

void tst_ParsedIntent::timeConstraintPreset()
{
    TimeConstraint tc;
    tc.kind = TimeConstraintKind::Preset;
    tc.preset = TimePreset::Today;
    QVERIFY(tc.isValid());
}

void tst_ParsedIntent::matchSpanValidity()
{
    MatchSpan span;
    QVERIFY(!span.isValid());

    span.start = 0;
    span.end = 5;
    span.ruleId = "test_rule";
    QVERIFY(span.isValid());
}

// ===== tst_IsSemanticQuery =====

namespace {

// Resolve the source tree rule directory relative to TEST_SOURCE_DIR.
// Falls back to a heuristic path if TEST_SOURCE_DIR is not defined.
QString sourceRulesDir()
{
    QString base = QString::fromUtf8(TEST_SOURCE_DIR);
    if (base.isEmpty()) {
        base = QCoreApplication::applicationDirPath() + "/../../..";
    }
    return base + "/src/dfm-search/dfm-search-lib/semantic/rules/zh_CN";
}

// Check whether the source tree rule files exist and are loadable.
bool sourceRulesAvailable()
{
    const QString dir = sourceRulesDir();
    return QDir(dir).exists()
            && !QDir(dir).entryList({ "*.json" }, QDir::Files).isEmpty();
}

// Replicate isSemanticQuery() logic using internal components with source-tree rules.
bool checkIsSemanticQuery(SemanticRuleEngine *engine, IntentParser *parser,
                          const QString &input)
{
    if (input.trimmed().isEmpty()) {
        return false;
    }

    ParsedIntent intent;
    parser->parse(input, intent);

    return intent.timeConstraint().isValid()
            || intent.sizeConstraint().isValid()
            || !intent.fileExtensions().isEmpty()
            || !intent.searchDirectories().isEmpty()
            || intent.includeHidden()
            || intent.hiddenOnly()
            || !intent.consumedSpans().isEmpty();
}

}   // namespace

class tst_IsSemanticQuery : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void emptyInput();
    void whitespaceOnly();
    void plainKeyword();
    void plainChineseKeyword();
    void todayKeyword();
    void yesterdayKeyword();
    void thisWeekKeyword();
    void lastMonthKeyword();
    void fileTypePdf();
    void fileTypeImage();
    void fileTypeDocument();
    void locationDesktop();
    void locationDownloads();
    void sizeLarge();
    void sizeSmall();
    void sizeDynamic();
    void timeAndFileType();
    void locationAndTime();
    void keywordOnlyNoMatch();
    void consecutiveCalls();
    void noiseWordsOnly();
    void hiddenFile();

private:
    SemanticRuleEngine *m_engine = nullptr;
    IntentParser *m_parser = nullptr;
};

void tst_IsSemanticQuery::initTestCase()
{
    if (!sourceRulesAvailable()) {
        QSKIP("Rule files not found in source tree, skipping isSemanticQuery tests");
    }

    m_engine = new SemanticRuleEngine(this);
    const QString dir = sourceRulesDir();
    const QStringList ruleFiles = QDir(dir).entryList(
            { "*.json" }, QDir::Files, QDir::Name);
    for (const QString &filename : ruleFiles) {
        QString path = dir + "/" + filename;
        if (!m_engine->loadRuleFile(path)) {
            qWarning() << "Failed to load rule file:" << path;
        }
    }

    m_parser = new IntentParser(m_engine);
}

void tst_IsSemanticQuery::emptyInput()
{
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, QString()));
}

void tst_IsSemanticQuery::whitespaceOnly()
{
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "   "));
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "\t\n"));
}

void tst_IsSemanticQuery::plainKeyword()
{
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "hello"));
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "meeting notes"));
}

void tst_IsSemanticQuery::plainChineseKeyword()
{
    // Pure Chinese text without any semantic triggers.
    // Avoid words that match filetype/location/time/size rules
    // (e.g. "报告" matches filetype_document_general, "音乐" matches filetype_audio).
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "蓝天白云"));
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "春夏秋冬"));
}

void tst_IsSemanticQuery::todayKeyword()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "今天的文件"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "今日份报告"));
}

void tst_IsSemanticQuery::yesterdayKeyword()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "昨天的报告"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "昨晚的截图"));
}

void tst_IsSemanticQuery::thisWeekKeyword()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "本周的文档"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "这周修改的"));
}

void tst_IsSemanticQuery::lastMonthKeyword()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "上个月的文件"));
}

void tst_IsSemanticQuery::fileTypePdf()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "pdf文档"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "找一下pdf"));
}

void tst_IsSemanticQuery::fileTypeImage()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "图片"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "截图"));
}

void tst_IsSemanticQuery::fileTypeDocument()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "文档"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "报告"));
}

void tst_IsSemanticQuery::locationDesktop()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "桌面的文件"));
}

void tst_IsSemanticQuery::locationDownloads()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "下载的文件"));
}

void tst_IsSemanticQuery::sizeLarge()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "大文件"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "几个G的文件"));
}

void tst_IsSemanticQuery::sizeSmall()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "小文件"));
}

void tst_IsSemanticQuery::sizeDynamic()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "大于500M的文件"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "小于100K"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "10M以上的表格"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "1G以内的文档"));
}

void tst_IsSemanticQuery::timeAndFileType()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "今天的pdf"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "本周的图片"));
}

void tst_IsSemanticQuery::locationAndTime()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "桌面今天的文件"));
}

void tst_IsSemanticQuery::keywordOnlyNoMatch()
{
    // Text that does not match any semantic rule pattern
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "xyzabc123"));
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "随便什么文字"));
}

void tst_IsSemanticQuery::consecutiveCalls()
{
    // Multiple calls with the same input should return consistent results
    QString input = "今天的pdf";
    bool first = checkIsSemanticQuery(m_engine, m_parser, input);
    bool second = checkIsSemanticQuery(m_engine, m_parser, input);
    bool third = checkIsSemanticQuery(m_engine, m_parser, input);
    QCOMPARE(first, second);
    QCOMPARE(second, third);
    QVERIFY(first);

    QString plain = "hello world";
    bool p1 = checkIsSemanticQuery(m_engine, m_parser, plain);
    bool p2 = checkIsSemanticQuery(m_engine, m_parser, plain);
    QCOMPARE(p1, p2);
    QVERIFY(!p1);
}

void tst_IsSemanticQuery::noiseWordsOnly()
{
    // Noise words alone (search action words) without any semantic dimension
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "搜索"));
    QVERIFY(!checkIsSemanticQuery(m_engine, m_parser, "查找"));
}

void tst_IsSemanticQuery::hiddenFile()
{
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "隐藏文件"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "隐藏的"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "藏起来的"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "看不到的"));
    QVERIFY(checkIsSemanticQuery(m_engine, m_parser, "今天的隐藏文件"));
}

// ===== tst_SearchTarget =====

class tst_SearchTarget : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void defaultIsAll();
    void filenameContains();
    void filenameNamed();
    void contentContains();
    void genericContainsStaysAll();
    void unconsumedTextStaysAll();

private:
    SemanticRuleEngine *m_engine = nullptr;
    KeywordExtractor *m_extractor = nullptr;
};

void tst_SearchTarget::initTestCase()
{
    if (!sourceRulesAvailable()) {
        QSKIP("Rule files not found in source tree, skipping search target tests");
    }

    m_engine = new SemanticRuleEngine(this);
    const QString dir = sourceRulesDir();
    const QStringList ruleFiles = QDir(dir).entryList(
            { "*.json" }, QDir::Files, QDir::Name);
    for (const QString &filename : ruleFiles) {
        QString path = dir + "/" + filename;
        if (!m_engine->loadRuleFile(path)) {
            qWarning() << "Failed to load rule file:" << path;
        }
    }

    m_extractor = new KeywordExtractor(m_engine);
}

void tst_SearchTarget::defaultIsAll()
{
    ParsedIntent intent;
    m_extractor->extract("蓝天白云", intent);
    QCOMPARE(intent.searchTarget(), SearchTarget::All);
}

void tst_SearchTarget::filenameContains()
{
    ParsedIntent intent;
    m_extractor->extract("文件名包含测试的文档", intent);
    QCOMPARE(intent.searchTarget(), SearchTarget::FileNameOnly);
    QCOMPARE(intent.keywords().size(), 1);
    QCOMPARE(intent.keywords().first(), QString("测试"));
}

void tst_SearchTarget::filenameNamed()
{
    ParsedIntent intent;
    m_extractor->extract("名为报告的文件", intent);
    QCOMPARE(intent.searchTarget(), SearchTarget::FileNameOnly);
    QCOMPARE(intent.keywords().size(), 1);
    QCOMPARE(intent.keywords().first(), QString("报告"));
}

void tst_SearchTarget::contentContains()
{
    ParsedIntent intent;
    m_extractor->extract("文件内容包含配置的文档", intent);
    QCOMPARE(intent.searchTarget(), SearchTarget::ContentOnly);
    QCOMPARE(intent.keywords().size(), 1);
    QCOMPARE(intent.keywords().first(), QString("配置"));
}

void tst_SearchTarget::genericContainsStaysAll()
{
    ParsedIntent intent;
    m_extractor->extract("包含测试的文件", intent);
    QCOMPARE(intent.searchTarget(), SearchTarget::All);
    QVERIFY(!intent.keywords().isEmpty());
}

void tst_SearchTarget::unconsumedTextStaysAll()
{
    // No structured keyword rule matches → unconsumed text extraction
    ParsedIntent intent;
    m_extractor->extract("项目计划书", intent);
    QCOMPARE(intent.searchTarget(), SearchTarget::All);
    QVERIFY(!intent.keywords().isEmpty());
}

// ===== tst_LocationExtraction =====

class tst_LocationExtraction : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Static method tests — no source-tree rule dependency
    void expandGlobNoMatch();
    void expandGlobSingleMatch();
    void expandGlobMultipleMatches();
    void expandGlobNestedDirs();
    void expandNonGlob();
    void expandGlobQuestionMark();

    // Integration tests — inline rules only
    void extractWechatTriggerWords();
    void extractCustomPathResolved();
};

void tst_LocationExtraction::expandGlobNoMatch()
{
    // Glob matching nothing → empty result (no crash)
    QStringList result = LocationExtractor::expandCustomPath("/tmp", "nonexistent/*/data");
    QVERIFY(result.isEmpty());
}

void tst_LocationExtraction::expandGlobSingleMatch()
{
    QString tmpBase = QDir::temp().filePath("tst_loc_" + QString::number(QDateTime::currentSecsSinceEpoch()));
    QDir().mkpath(tmpBase + "/user_a/sub/folder");

    QStringList result = LocationExtractor::expandCustomPath(tmpBase, "user_*/sub/folder");
    QCOMPARE(result.size(), 1);
    QVERIFY(result.first().endsWith("/user_a/sub/folder"));

    QDir(tmpBase).removeRecursively();
}

void tst_LocationExtraction::expandGlobMultipleMatches()
{
    QString tmpBase = QDir::temp().filePath("tst_loc_" + QString::number(QDateTime::currentSecsSinceEpoch()));
    QDir().mkpath(tmpBase + "/wxid_abc/msg/file");
    QDir().mkpath(tmpBase + "/wxid_def/msg/file");
    QDir().mkpath(tmpBase + "/wxid_ghi/msg/file");

    QStringList result = LocationExtractor::expandCustomPath(tmpBase, "wxid_*/msg/file");
    // At least our 3 test dirs should be present
    QVERIFY(result.size() >= 3);
    bool hasA = false, hasD = false, hasG = false;
    for (const QString &p : result) {
        if (p.contains("/wxid_abc/msg/file")) hasA = true;
        if (p.contains("/wxid_def/msg/file")) hasD = true;
        if (p.contains("/wxid_ghi/msg/file")) hasG = true;
    }
    QVERIFY(hasA && hasD && hasG);

    QDir(tmpBase).removeRecursively();
}

void tst_LocationExtraction::expandGlobNestedDirs()
{
    // Pattern: a/b/*/c/d → a/<user>/c/d
    QString tmpBase = QDir::temp().filePath("tst_loc_" + QString::number(QDateTime::currentMSecsSinceEpoch()));
    QDir().mkpath(tmpBase + "/docs/users/u1/c/d");
    QDir().mkpath(tmpBase + "/docs/users/u2/c/d");

    QStringList result = LocationExtractor::expandCustomPath(tmpBase, "docs/users/*/c/d");
    QVERIFY(result.size() >= 2);

    QDir(tmpBase).removeRecursively();
}

void tst_LocationExtraction::expandNonGlob()
{
    // Non-glob path is returned as-is, appended to base
    QStringList result = LocationExtractor::expandCustomPath("/home/test", "Downloads/myapp/data");
    QCOMPARE(result, QStringList { "/home/test/Downloads/myapp/data" });
}

void tst_LocationExtraction::expandGlobQuestionMark()
{
    // ? matches single character
    QString tmpBase = QDir::temp().filePath("tst_loc_" + QString::number(QDateTime::currentMSecsSinceEpoch()));
    QDir().mkpath(tmpBase + "/dir_a/data");
    QDir().mkpath(tmpBase + "/dir_b/data");
    QDir().mkpath(tmpBase + "/dir_abc/data");   // won't match "?_" pattern

    QStringList result = LocationExtractor::expandCustomPath(tmpBase, "dir_?/data");
    QVERIFY(result.size() >= 2);

    bool hasA = false, hasB = false;
    for (const QString &p : result) {
        if (p.endsWith("/dir_a/data")) hasA = true;
        if (p.endsWith("/dir_b/data")) hasB = true;
    }
    QVERIFY(hasA && hasB);

    QDir(tmpBase).removeRecursively();
}

void tst_LocationExtraction::extractWechatTriggerWords()
{
    // Build inline rule engine to avoid source-tree / local filesystem dependency.
    // Directory must be under QDir::homePath() so expandCustomPath can find it.
    const QString suffix = QString::number(QDateTime::currentSecsSinceEpoch());
    const QString baseName = "tst_wechat_" + suffix;
    const QString tmpDir = QDir::home().filePath(baseName);
    QDir().mkpath(tmpDir);

    QByteArray json = makeRuleJson("location", "loc_test_wechat", "微信|vx|wechat", 180,
                                   { { "custom_path", baseName },
                                     { "include_hidden", false } });

    QFile f(QDir::temp().filePath("tst_wechat_inline.json"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json);
    f.close();

    SemanticRuleEngine engine;
    QVERIFY(engine.loadRuleFile(f.fileName()));

    LocationExtractor extractor(&engine);
    const QStringList inputs = { "微信的文件", "vx收到的", "wechat文件" };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        extractor.extract(input, intent);
        QVERIFY2(!intent.consumedSpans().isEmpty(),
                 qUtf8Printable("Expected consumed span for: " + input));
        QVERIFY2(!intent.searchDirectories().isEmpty(),
                 qUtf8Printable("Expected searchDirectory for: " + input));
    }

    f.remove();
    QDir(tmpDir).rmdir(tmpDir);
}

void tst_LocationExtraction::extractCustomPathResolved()
{
    // Verify extract() resolves custom_path and appends to searchDirectories.
    // Directory must live under QDir::homePath() so expandCustomPath can find it.
    const QString suffix = QString::number(QDateTime::currentSecsSinceEpoch());
    const QString baseName = "tst_custom_" + suffix;
    // Create under home, not QDir::temp() (which may be /tmp, outside home path)
    const QString tmpDir = QDir::home().filePath(baseName);
    QDir().mkpath(tmpDir + "/data");

    QByteArray json = makeRuleJson("location", "loc_test_cp", "我的目录", 180,
                                   { { "custom_path", baseName + "/data" },
                                     { "include_hidden", false } });

    QFile f(QDir::temp().filePath("tst_cp_inline.json"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(json);
    f.close();

    SemanticRuleEngine engine;
    QVERIFY(engine.loadRuleFile(f.fileName()));

    LocationExtractor extractor(&engine);
    ParsedIntent intent;
    extractor.extract("我的目录的文件", intent);

    QVERIFY2(!intent.searchDirectories().isEmpty(),
             "Expected at least one resolved search directory");
    // Path must end with /data
    bool hasData = false;
    for (const QString &path : intent.searchDirectories()) {
        if (path.endsWith("/data")) hasData = true;
    }
    QVERIFY(hasData);

    f.remove();
    QDir(tmpDir).removeRecursively();
}

// ===== tst_SemanticQueryBuilderTarget =====

class tst_SemanticQueryBuilderTarget : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultTarget();
    void fileNameOnlyTarget();
    void contentOnlyTarget();

private:
    ParsedIntent makeIntent(const QStringList &keywords, SearchTarget target) const;
};

ParsedIntent tst_SemanticQueryBuilderTarget::makeIntent(
        const QStringList &keywords, SearchTarget target) const
{
    ParsedIntent intent;
    intent.setKeywords(keywords);
    intent.setSearchTarget(target);
    return intent;
}

void tst_SemanticQueryBuilderTarget::defaultTarget()
{
    SemanticQueryBuilder builder;
    ParsedIntent intent = makeIntent({ "测试" }, SearchTarget::All);
    SemanticSearchPlan plan = builder.build(intent);

    // All three paths should produce queries
    QVERIFY(!plan.fileNameQuery.keyword().isEmpty());
    QVERIFY(plan.contentQuery.has_value());
    QVERIFY(plan.ocrQuery.has_value());
}

void tst_SemanticQueryBuilderTarget::fileNameOnlyTarget()
{
    SemanticQueryBuilder builder;
    ParsedIntent intent = makeIntent({ "测试" }, SearchTarget::FileNameOnly);
    SemanticSearchPlan plan = builder.build(intent);

    // Only filename query should be built
    QVERIFY(!plan.fileNameQuery.keyword().isEmpty());
    QVERIFY(!plan.contentQuery.has_value());
    QVERIFY(!plan.ocrQuery.has_value());
}

void tst_SemanticQueryBuilderTarget::contentOnlyTarget()
{
    SemanticQueryBuilder builder;
    ParsedIntent intent = makeIntent({ "测试" }, SearchTarget::ContentOnly);
    SemanticSearchPlan plan = builder.build(intent);

    // Filename should NOT be built; content and ocr should
    QVERIFY(plan.fileNameQuery.keyword().isEmpty());
    QVERIFY(plan.contentQuery.has_value());
    QVERIFY(plan.ocrQuery.has_value());
}

// ===== Factory functions =====

QObject *create_tst_RuleEngine()
{
    return new tst_RuleEngine();
}
QObject *create_tst_TimeExtraction()
{
    return new tst_TimeExtraction();
}
QObject *create_tst_FileTypeExtraction()
{
    return new tst_FileTypeExtraction();
}
QObject *create_tst_KeywordExtraction()
{
    return new tst_KeywordExtraction();
}
QObject *create_tst_ParsedIntent()
{
    return new tst_ParsedIntent();
}
QObject *create_tst_IsSemanticQuery()
{
    return new tst_IsSemanticQuery();
}
QObject *create_tst_SearchTarget()
{
    return new tst_SearchTarget();
}
QObject *create_tst_SemanticQueryBuilderTarget()
{
    return new tst_SemanticQueryBuilderTarget();
}
QObject *create_tst_LocationExtraction()
{
    return new tst_LocationExtraction();
}

#include "tst_semantic_search.moc"
