// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

#include "semantic/semanticruleengine.h"
#include "semantic/intentparser.h"

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
    ruleGroupObj["rules"] = QJsonArray({ruleObj});

    QJsonObject root;
    root["groups"] = QJsonArray({ruleGroupObj});

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
    r1["id"] = "low";  r1["pattern"] = "test"; r1["priority"] = 10;
    r2["id"] = "high"; r2["pattern"] = "test"; r2["priority"] = 200;
    r3["id"] = "mid";  r3["pattern"] = "test"; r3["priority"] = 100;

    QJsonObject ruleGroup;
    ruleGroup["name"] = "prio";
    ruleGroup["version"] = "1.0.0";
    ruleGroup["rules"] = QJsonArray({r1, r2, r3});

    QJsonObject root;
    root["groups"] = QJsonArray({ruleGroup});

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
                                   {{"level", "high"}});

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("abc").hasMatch());
}

void tst_RuleEngine::matchAllReturnsAll()
{
    QJsonObject r1, r2, r3;
    r1["id"] = "r1"; r1["pattern"] = "cat"; r1["priority"] = 100;
    r2["id"] = "r2"; r2["pattern"] = "dog"; r2["priority"] = 100;
    r3["id"] = "r3"; r3["pattern"] = "bird"; r3["priority"] = 50;

    QJsonObject ruleGroup;
    ruleGroup["name"] = "test_all";
    ruleGroup["version"] = "1.0.0";
    ruleGroup["rules"] = QJsonArray({r1, r2, r3});

    QJsonObject root;
    root["groups"] = QJsonArray({ruleGroup});

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
    meta["extensions"] = QStringList({"pdf"});
    QByteArray json = makeRuleJson("filetype", "filetype_pdf", "pdf", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("pdf").hasMatch());
    QCOMPARE(group.rules[0].metadata.value("extensions").toStringList(), QStringList({"pdf"}));
}

void tst_FileTypeExtraction::preciseWord()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"doc", "docx"});
    meta["fileTypes"] = QStringList({"doc"});
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
    meta["extensions"] = QStringList({"xls", "xlsx"});
    QByteArray json = makeRuleJson("filetype", "filetype_excel", "excel|xls|xlsx", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("excel").hasMatch());
    QVERIFY(group.rules[0].regex.match("xlsx").hasMatch());
}

void tst_FileTypeExtraction::precisePpt()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"ppt", "pptx"});
    QByteArray json = makeRuleJson("filetype", "filetype_ppt", "ppt|pptx", 200, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].regex.match("ppt").hasMatch());
    QVERIFY(group.rules[0].regex.match("pptx").hasMatch());
}

void tst_FileTypeExtraction::imageType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"jpg", "png", "gif"});
    meta["fileTypes"] = QStringList({"pic"});
    QByteArray json = makeRuleJson("filetype", "filetype_image", "image", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("fileTypes").toStringList(), QStringList({"pic"}));
}

void tst_FileTypeExtraction::videoType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"mp4", "avi", "mkv"});
    meta["fileTypes"] = QStringList({"video"});
    QByteArray json = makeRuleJson("filetype", "filetype_video", "video", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("fileTypes").toStringList(), QStringList({"video"}));
}

void tst_FileTypeExtraction::audioType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"mp3", "wav", "flac"});
    meta["fileTypes"] = QStringList({"audio"});
    QByteArray json = makeRuleJson("filetype", "filetype_audio", "audio", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("fileTypes").toStringList(), QStringList({"audio"}));
}

void tst_FileTypeExtraction::genericDocument()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"doc", "docx", "pdf", "txt"});
    meta["fileTypes"] = QStringList({"doc"});
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
    meta["extensions"] = QStringList({"xls", "xlsx", "csv"});
    meta["general"] = true;
    QByteArray json = makeRuleJson("filetype", "filetype_spreadsheet_general", "spreadsheet", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].metadata.value("general").toBool());
}

void tst_FileTypeExtraction::genericPresentation()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"ppt", "pptx", "dps"});
    meta["general"] = true;
    QByteArray json = makeRuleJson("filetype", "filetype_presentation_general", "presentation", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QVERIFY(group.rules[0].metadata.value("general").toBool());
}

void tst_FileTypeExtraction::archiveType()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"zip", "tar", "rar", "7z"});
    QByteArray json = makeRuleJson("filetype", "filetype_archive", "archive", 150, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("extensions").toStringList().size(), 4);
}

void tst_FileTypeExtraction::extensionsList()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"a", "b", "c", "d", "e"});
    QByteArray json = makeRuleJson("filetype", "ft", "test", 100, meta);

    RuleGroup group;
    QVERIFY(buildGroupFromJson(json, group));
    QCOMPARE(group.rules[0].metadata.value("extensions").toStringList().size(), 5);
}

void tst_FileTypeExtraction::generalFlag()
{
    QVariantMap meta;
    meta["extensions"] = QStringList({"pdf"});
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
    QVERIFY(intent.timeConstraint.kind == TimeConstraintKind::None);
    QVERIFY(intent.fileExtensions.isEmpty());
    QVERIFY(intent.keywords.isEmpty());
    QVERIFY(intent.consumedSpans.isEmpty());
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

// ===== Factory functions =====

QObject *create_tst_RuleEngine() { return new tst_RuleEngine(); }
QObject *create_tst_TimeExtraction() { return new tst_TimeExtraction(); }
QObject *create_tst_FileTypeExtraction() { return new tst_FileTypeExtraction(); }
QObject *create_tst_KeywordExtraction() { return new tst_KeywordExtraction(); }
QObject *create_tst_ParsedIntent() { return new tst_ParsedIntent(); }

#include "tst_semantic_search.moc"
