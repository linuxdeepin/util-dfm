// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QSet>
#include <QTest>

#include "semantic/intentparser.h"
#include "semantic/semanticruleengine.h"

using namespace DFMSEARCH;

static QString rulesDir()
{
    return QStringLiteral(TEST_SOURCE_DIR) + QStringLiteral(
            "/src/dfm-search/dfm-search-lib/semantic/rules/zh_CN");
}

// Helper: compare two QStringList as sets (order-independent)
static bool setEquals(const QStringList &a, const QStringList &b)
{
    return QSet<QString>(a.begin(), a.end()) == QSet<QString>(b.begin(), b.end());
}

class tst_ChineseNLP : public QObject
{
    Q_OBJECT

private:
    SemanticRuleEngine *m_engine = nullptr;
    IntentParser *m_parser = nullptr;

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();

    // Time preset tests
    void timePreset_today();
    void timePreset_today_alt();
    void timePreset_yesterday();
    void timePreset_yesterday_variants();
    void timePreset_dayBeforeYesterday();
    void timePreset_thisWeek_variants();
    void timePreset_lastWeek_variants();
    void timePreset_thisMonth_variants();
    void timePreset_lastMonth_variants();
    void timePreset_thisYear_variants();
    void timePreset_lastYear_variants();

    // Time custom tests
    void timeCustom_year();
    void timeCustom_year_twoDigit();
    void timeCustom_month();
    void timeCustom_yearMonth();
    void timeCustom_yearMonth_separators();
    void timeCustom_date();
    void timeCustom_dateSpoken();
    void timeCustom_fullDate();
    void timeCustom_fullDate_separators();
    void timeCustom_yesterday_variants_all();
    void timeCustom_lastYear_extra();

    // File type tests
    void fileType_precise_pdf();
    void fileType_precise_word();
    void fileType_precise_excel();
    void fileType_precise_ppt();
    void fileType_category_image_variants();
    void fileType_category_video_variants();
    void fileType_category_audio_variants();
    void fileType_category_archive();
    void fileType_category_application();
    void fileType_category_designSource();
    void fileType_general_document();
    void fileType_general_spreadsheet();
    void fileType_general_presentation();

    // Filetype all-synonyms tests (from requirements)
    void fileType_document_general_allSynonyms();
    void fileType_spreadsheet_general_allSynonyms();
    void filetype_presentation_general_allSynonyms();
    void fileType_image_allSynonyms();
    void fileType_video_allSynonyms();
    void fileType_audio_allSynonyms();
    void fileType_archive_allSynonyms();
    void fileType_application_allSynonyms();
    void fileType_design_source_allSynonyms();

    // Combined time+type tests
    void combined_fullDateAndType();
    void combined_monthAndType();
    void combined_yearAndType();

    // Size tests
    void size_fuzzy_large();
    void size_fuzzy_large_synonyms();
    void size_fuzzy_small();
    void size_dynamic_min();
    void size_dynamic_max();
    void size_dynamic_between();
    void size_combined_withTime();
    void size_combined_withType();
    void size_combined_full();

    // Relative time tests
    void timeRelative_justNow();
    void timeRelative_justNow_synonyms();
    void timeRelative_recentDays();
    void timeRelative_recentDays_synonyms();
    void timeRelative_pastFewDays();
    void timeRelative_pastFewDays_synonyms();
    void timeRelative_aWhileAgo();
    void timeRelative_aWhileAgo_synonyms();
    void timeRelative_priority_vs_preset();

    // Action behavior tests
    void action_create_birthTime();
    void action_create_synonyms();
    void action_modify_modifyTime();
    void action_modify_synonyms();
    void action_default_unspecified();
    void action_combined_withTime_create();
    void action_combined_withTime_modify();

    // Keyword tests
    void keyword_contains_single();
    void keyword_contains_multi();
    void keyword_named();
    void keyword_contentHas();
    void keyword_contentHas_multi();

    // Noise + unconsumed text tests
    void noise_action_words();
    void noise_polite_words();
    void noise_suffix_words();

    // End-to-end combined tests
    void combined_timeAndFiletype();
    void combined_timeAndFiletype_multi();
    void combined_timeAndFiletype_all();
    void combined_timeAndKeyword();
    void combined_filetypeAndKeyword();
    void combined_timeAndFiletypeAndKeyword();
    void combined_noiseStripping();
    void combined_fullSentence();
    void combined_noTime();
    void combined_onlyKeyword();
    void combined_generalSuppressed();
    void combined_contentHasAndType();
};

void tst_ChineseNLP::initTestCase()
{
    // Initialize QCoreApplication for Qt test framework
    if (!QCoreApplication::instance()) {
        int argc = 0;
        new QCoreApplication(argc, nullptr);
    }

    m_engine = new SemanticRuleEngine(this);

    // Load all 6 rule files
    const QString dir = rulesDir();
    QVERIFY2(QDir(dir).exists(), qPrintable(QStringLiteral("Rules dir not found: ") + dir));

    const QStringList files = { "noise_rules.json", "time_rules.json",
                                "filetype_rules.json", "keyword_rules.json",
                                "size_rules.json", "action_rules.json" };
    for (const QString &f : files) {
        const QString path = dir + QLatin1Char('/') + f;
        bool ok = m_engine->loadRuleFile(path);
        QVERIFY2(ok, qPrintable(QStringLiteral("Failed to load: ") + path));
    }

    // Verify all groups loaded
    QVERIFY(m_engine->hasGroup("time"));
    QVERIFY(m_engine->hasGroup("filetype"));
    QVERIFY(m_engine->hasGroup("keyword"));
    QVERIFY(m_engine->hasGroup("noise"));
    QVERIFY(m_engine->hasGroup("size"));
    QVERIFY(m_engine->hasGroup("action"));

    const QStringList groups = m_engine->groupNames();
    QCOMPARE(groups.size(), 6);

    m_parser = new IntentParser(m_engine);

    // Verify default extractors are initialized
    QStringList names = m_parser->extractorNames();
    QCOMPARE(names.size(), 5);
    QVERIFY(names.contains("time"));
    QVERIFY(names.contains("filetype"));
    QVERIFY(names.contains("size"));
    QVERIFY(names.contains("action"));
    QVERIFY(names.contains("keyword"));
}

void tst_ChineseNLP::cleanupTestCase()
{
    delete m_parser;
    m_parser = nullptr;
}

void tst_ChineseNLP::init()
{
    // Each test gets a fresh parse — no shared state between tests
}

// ===== Time Preset Tests =====

void tst_ChineseNLP::timePreset_today()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天的文件"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
}

void tst_ChineseNLP::timePreset_today_alt()
{
    // 今日 and 今日份
    ParsedIntent intent1;
    m_parser->parse(QStringLiteral("今日的文档"), intent1);
    QCOMPARE(intent1.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent1.timeConstraint.preset, TimePreset::Today);

    ParsedIntent intent2;
    m_parser->parse(QStringLiteral("今日份图片"), intent2);
    QCOMPARE(intent2.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent2.timeConstraint.preset, TimePreset::Today);
}

void tst_ChineseNLP::timePreset_yesterday()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("昨天的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Yesterday);
}

void tst_ChineseNLP::timePreset_yesterday_variants()
{
    const QStringList inputs = { QStringLiteral("昨日"), QStringLiteral("昨晚"),
                                  QStringLiteral("昨天上午"), QStringLiteral("昨天下午"),
                                  QStringLiteral("昨天晚上") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
        QCOMPARE(intent.timeConstraint.preset, TimePreset::Yesterday);
    }
}

void tst_ChineseNLP::timePreset_dayBeforeYesterday()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("前天的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::DayBeforeYesterday);
}

void tst_ChineseNLP::timePreset_thisWeek_variants()
{
    const QStringList inputs = { QStringLiteral("本周"), QStringLiteral("这周"),
                                  QStringLiteral("这个星期"), QStringLiteral("这一个星期") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
        QCOMPARE(intent.timeConstraint.preset, TimePreset::ThisWeek);
    }
}

void tst_ChineseNLP::timePreset_lastWeek_variants()
{
    const QStringList inputs = { QStringLiteral("上周"), QStringLiteral("上个星期"),
                                  QStringLiteral("上星期"), QStringLiteral("上一个星期") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
        QCOMPARE(intent.timeConstraint.preset, TimePreset::LastWeek);
    }
}

void tst_ChineseNLP::timePreset_thisMonth_variants()
{
    const QStringList inputs = { QStringLiteral("本月"), QStringLiteral("这个月"),
                                  QStringLiteral("当月") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
        QCOMPARE(intent.timeConstraint.preset, TimePreset::ThisMonth);
    }
}

void tst_ChineseNLP::timePreset_lastMonth_variants()
{
    const QStringList inputs = { QStringLiteral("上个月"), QStringLiteral("上月") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
        QCOMPARE(intent.timeConstraint.preset, TimePreset::LastMonth);
    }
}

void tst_ChineseNLP::timePreset_thisYear_variants()
{
    const QStringList inputs = { QStringLiteral("今年"), QStringLiteral("本年"),
                                  QStringLiteral("这年") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
        QCOMPARE(intent.timeConstraint.preset, TimePreset::ThisYear);
    }
}

void tst_ChineseNLP::timePreset_lastYear_variants()
{
    const QStringList inputs = { QStringLiteral("去年"), QStringLiteral("上一年") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
        QCOMPARE(intent.timeConstraint.preset, TimePreset::LastYear);
    }
}

// ===== Time Custom Tests =====

void tst_ChineseNLP::timeCustom_year()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("2025年的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 1);
    QCOMPARE(intent.timeConstraint.customStart.date().day(), 1);
    QCOMPARE(intent.timeConstraint.customEnd.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customEnd.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customEnd.date().day(), 31);
}

void tst_ChineseNLP::timeCustom_year_twoDigit()
{
    // Two-digit year: 25 -> 2025
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("25年的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customEnd.date().year(), 2025);
}

// ===== File Type Tests =====

void tst_ChineseNLP::fileType_precise_pdf()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("pdf"), intent);
    QVERIFY(intent.fileExtensions.contains("pdf"));
    QCOMPARE(intent.fileExtensions.size(), 1);
}

void tst_ChineseNLP::fileType_precise_word()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("word"), intent);
    QVERIFY(setEquals(intent.fileExtensions, QStringList { "doc", "docx" }));
}

void tst_ChineseNLP::fileType_precise_excel()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("excel"), intent);
    QVERIFY(setEquals(intent.fileExtensions, QStringList { "xls", "xlsx" }));
}

void tst_ChineseNLP::fileType_precise_ppt()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("ppt"), intent);
    QVERIFY(setEquals(intent.fileExtensions, QStringList { "ppt", "pptx" }));
}

void tst_ChineseNLP::fileType_category_image_variants()
{
    const QStringList inputs = { QStringLiteral("图片"), QStringLiteral("照片"),
                                  QStringLiteral("截图"), QStringLiteral("壁纸"),
                                  QStringLiteral("海报"), QStringLiteral("相片"),
                                  QStringLiteral("表情包"), QStringLiteral("图") };
    const QStringList expectedExts = { "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg" };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(setEquals(intent.fileExtensions, expectedExts),
                 qPrintable(QStringLiteral("Failed for input: ") + input));
    }
}

void tst_ChineseNLP::fileType_category_video_variants()
{
    const QStringList inputs = { QStringLiteral("视频"), QStringLiteral("录像"),
                                  QStringLiteral("电影"), QStringLiteral("动画"),
                                  QStringLiteral("短片"), QStringLiteral("片子") };
    const QStringList expectedExts = { "mp4", "avi", "mkv", "mov", "flv", "wmv", "webm" };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(setEquals(intent.fileExtensions, expectedExts),
                 qPrintable(QStringLiteral("Failed for input: ") + input));
    }
}

void tst_ChineseNLP::fileType_category_audio_variants()
{
    const QStringList inputs = { QStringLiteral("音频"), QStringLiteral("音乐"),
                                  QStringLiteral("录音"), QStringLiteral("歌"),
                                  QStringLiteral("语音") };
    const QStringList expectedExts = { "mp3", "wav", "flac", "aac", "ogg", "m4a" };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(setEquals(intent.fileExtensions, expectedExts),
                 qPrintable(QStringLiteral("Failed for input: ") + input));
    }
}

void tst_ChineseNLP::fileType_category_archive()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("压缩包"), intent);
    QVERIFY(intent.fileExtensions.contains("zip"));
    QVERIFY(intent.fileExtensions.contains("tar.gz"));
    QVERIFY(intent.fileExtensions.contains("rar"));
    QVERIFY(intent.fileExtensions.contains("7z"));
}

void tst_ChineseNLP::fileType_category_application()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("安装包"), intent);
    QVERIFY(intent.fileExtensions.contains("deb"));
    QVERIFY(intent.fileExtensions.contains("sh"));
}

void tst_ChineseNLP::fileType_category_designSource()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("源文件"), intent);
    QVERIFY(intent.fileExtensions.contains("psd"));
    QVERIFY(intent.fileExtensions.contains("ai"));
}

void tst_ChineseNLP::fileType_general_document()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("文档"), intent);
    const QStringList expectedExts = { "doc", "docx", "pdf", "txt", "wps", "rtf", "md", "odt" };
    QVERIFY(setEquals(intent.fileExtensions, expectedExts));
}

void tst_ChineseNLP::fileType_general_spreadsheet()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("表格"), intent);
    QVERIFY(intent.fileExtensions.contains("xls"));
    QVERIFY(intent.fileExtensions.contains("xlsx"));
    QVERIFY(intent.fileExtensions.contains("csv"));
}

void tst_ChineseNLP::fileType_general_presentation()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("幻灯片"), intent);
    QVERIFY(intent.fileExtensions.contains("ppt"));
    QVERIFY(intent.fileExtensions.contains("pptx"));
}

// ===== Keyword Tests =====

void tst_ChineseNLP::keyword_contains_single()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("包含会议记录的文档"), intent);
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("会议记录"));
}

void tst_ChineseNLP::keyword_contains_multi()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("包含预算和收入的报告"), intent);
    QCOMPARE(intent.keywords.size(), 2);
    QVERIFY(intent.keywords.contains("预算"));
    QVERIFY(intent.keywords.contains("收入"));
}

void tst_ChineseNLP::keyword_named()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("名为方案A的文档"), intent);
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("方案A"));
}

void tst_ChineseNLP::keyword_contentHas()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("内容包含数据分析的报告"), intent);
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("数据分析"));
}

void tst_ChineseNLP::keyword_contentHas_multi()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("内容含有产品规划和市场调研的报告"), intent);
    QCOMPARE(intent.keywords.size(), 2);
    QVERIFY(intent.keywords.contains("产品规划"));
    QVERIFY(intent.keywords.contains("市场调研"));
}

// ===== Noise + Unconsumed Text Tests =====

void tst_ChineseNLP::noise_action_words()
{
    // "搜索" is noise; "上周" is time; "图片" is filetype
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("搜索上周的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::LastWeek);
    // Filetype should be matched
    QVERIFY(!intent.fileExtensions.isEmpty());
    // No keywords since all text is consumed
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::noise_polite_words()
{
    // "请帮我找" consumed as noise; "今天" time; "文档" filetype
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("请帮我找今天的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    QVERIFY(!intent.fileExtensions.isEmpty());
    // All text consumed by noise + time + filetype
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::noise_suffix_words()
{
    // "昨天上午" time; "的照片" noise_suffix
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("昨天上午的照片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Yesterday);
    QVERIFY(!intent.fileExtensions.isEmpty());
    QVERIFY(intent.keywords.isEmpty());
}

// ===== End-to-End Combined Tests =====

void tst_ChineseNLP::combined_timeAndFiletype()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    const QStringList imageExts = { "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg" };
    QVERIFY(setEquals(intent.fileExtensions, imageExts));
    // "的" is consumed by noise_suffix "的图片"
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::combined_timeAndFiletype_multi()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天的图片和视频"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    // Should contain both image and video extensions
    QVERIFY(intent.fileExtensions.contains("jpg"));
    QVERIFY(intent.fileExtensions.contains("png"));
    QVERIFY(intent.fileExtensions.contains("mp4"));
    QVERIFY(intent.fileExtensions.contains("mkv"));
    QVERIFY(intent.fileExtensions.contains("avi"));
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::combined_timeAndFiletype_all()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天的图片和视频和音频"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    QVERIFY(intent.fileExtensions.contains("jpg"));
    QVERIFY(intent.fileExtensions.contains("mp4"));
    QVERIFY(intent.fileExtensions.contains("mp3"));
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::combined_timeAndKeyword()
{
    // "今天" time, "包含会议记录的" keyword pattern, "文档" filetype
    // But since keyword pattern matches, filetype_document_general is skipped
    // because keyword_extractor returns early. The filetype extractor runs
    // before keyword extractor and matches "文档".
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天包含会议记录的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    QVERIFY(intent.keywords.contains("会议记录"));
    // "文档" matches filetype_document_general
    QVERIFY(!intent.fileExtensions.isEmpty());
}

void tst_ChineseNLP::combined_filetypeAndKeyword()
{
    // "名为方案A的" → keyword "方案A"; "pdf" → filetype
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("名为方案A的pdf"), intent);
    QVERIFY(intent.fileExtensions.contains("pdf"));
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("方案A"));
}

void tst_ChineseNLP::combined_timeAndFiletypeAndKeyword()
{
    // "昨天" time, "视频" filetype (priority 150, non-general),
    // "包含报告的" keyword → "报告"
    // Note: "报告" also matches filetype_document_general but it's general
    // and gets skipped since video exts are already in seenExtensions
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("昨天的视频和包含报告的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Yesterday);
    // Video extensions
    QVERIFY(intent.fileExtensions.contains("mp4"));
    QVERIFY(intent.fileExtensions.contains("avi"));
    // Keyword extracted from structured pattern
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("报告"));
}

void tst_ChineseNLP::combined_noiseStripping()
{
    // "帮我找" noise_action, "今天" time, "会议" unconsumed → keyword, "文档" filetype
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("帮我找今天的会议文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    QVERIFY(!intent.fileExtensions.isEmpty());
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("会议"));
}

void tst_ChineseNLP::combined_fullSentence()
{
    // "请搜索上周的图片和视频" → noise(请,搜索) + time(上周) + filetype(图片,视频)
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("请搜索上周的图片和视频"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::LastWeek);
    QVERIFY(intent.fileExtensions.contains("jpg"));
    QVERIFY(intent.fileExtensions.contains("mp4"));
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::combined_noTime()
{
    // No time, keyword from "包含数据", filetype from "表格"
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("包含数据的表格"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::None);
    QVERIFY(!intent.fileExtensions.isEmpty());
    QVERIFY(intent.fileExtensions.contains("xls"));
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("数据"));
}

void tst_ChineseNLP::combined_onlyKeyword()
{
    // No time, no filetype, only unconsumed text as keyword
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("会议记录"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::None);
    QVERIFY(intent.fileExtensions.isEmpty());
    QCOMPARE(intent.keywords.size(), 1);
    QCOMPARE(intent.keywords.first(), QString("会议记录"));
}

void tst_ChineseNLP::combined_generalSuppressed()
{
    // "pdf" precise (priority 200) wins; "文档" general (priority 100) suppressed
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("pdf文档"), intent);
    QCOMPARE(intent.fileExtensions.size(), 1);
    QCOMPARE(intent.fileExtensions.first(), QString("pdf"));
}

void tst_ChineseNLP::combined_contentHasAndType()
{
    // "内容包含测试的报告" → keyword "测试", filetype "报告" (document general)
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("内容包含测试的报告"), intent);
    QVERIFY(intent.keywords.contains("测试"));
    QVERIFY(!intent.fileExtensions.isEmpty());
    // "报告" is in filetype_document_general pattern
    QVERIFY(intent.fileExtensions.contains("doc"));
    QVERIFY(intent.fileExtensions.contains("pdf"));
}

// ===== New Time Custom Tests =====

void tst_ChineseNLP::timeCustom_month()
{
    // "12月" → this month 12
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("12月的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customStart.date().day(), 1);
    // End should be last day of December
    QCOMPARE(intent.timeConstraint.customEnd.date().month(), 12);

    // "5月份" — same month, different syntax
    ParsedIntent intent2;
    m_parser->parse(QStringLiteral("5月份的图片"), intent2);
    QCOMPARE(intent2.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent2.timeConstraint.customStart.date().month(), 5);
}

void tst_ChineseNLP::timeCustom_yearMonth()
{
    // "2025年12月" → year=2025, month=12
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("2025年12月的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customStart.date().day(), 1);
    QCOMPARE(intent.timeConstraint.customEnd.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customEnd.date().month(), 12);
}

void tst_ChineseNLP::timeCustom_yearMonth_separators()
{
    // "2025-12" — dash separator
    ParsedIntent intent1;
    m_parser->parse(QStringLiteral("2025-12的图片"), intent1);
    QCOMPARE(intent1.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent1.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent1.timeConstraint.customStart.date().month(), 12);

    // "2025/12" — slash separator
    ParsedIntent intent2;
    m_parser->parse(QStringLiteral("2025/12的视频"), intent2);
    QCOMPARE(intent2.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent2.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent2.timeConstraint.customStart.date().month(), 12);

    // "25.12" — dot separator, 2-digit year
    ParsedIntent intent3;
    m_parser->parse(QStringLiteral("25.12的文件"), intent3);
    QCOMPARE(intent3.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent3.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent3.timeConstraint.customStart.date().month(), 12);
}

void tst_ChineseNLP::timeCustom_date()
{
    // "12月5日" → this year, Dec 5
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("12月5日的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customStart.date().day(), 5);
    QCOMPARE(intent.timeConstraint.customEnd.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customEnd.date().day(), 5);
    QCOMPARE(intent.timeConstraint.customStart.date().year(), QDate::currentDate().year());
}

void tst_ChineseNLP::timeCustom_dateSpoken()
{
    // "3月8号" — spoken form with 号
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("3月8号的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 3);
    QCOMPARE(intent.timeConstraint.customStart.date().day(), 8);
}

void tst_ChineseNLP::timeCustom_fullDate()
{
    // "2025年12月30日" — the specific example from requirements
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("2025年12月30日的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customStart.date().day(), 30);
    QCOMPARE(intent.timeConstraint.customEnd.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customEnd.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customEnd.date().day(), 30);
    // Verify time boundaries
    QCOMPARE(intent.timeConstraint.customStart.time().hour(), 0);
    QCOMPARE(intent.timeConstraint.customEnd.time().hour(), 23);
}

void tst_ChineseNLP::timeCustom_fullDate_separators()
{
    // "2025-12-05" — dash format
    ParsedIntent intent1;
    m_parser->parse(QStringLiteral("2025-12-05的文档"), intent1);
    QCOMPARE(intent1.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent1.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent1.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent1.timeConstraint.customStart.date().day(), 5);

    // "2025/12/5" — slash format (no leading zero)
    ParsedIntent intent2;
    m_parser->parse(QStringLiteral("2025/12/5的文件"), intent2);
    QCOMPARE(intent2.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent2.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent2.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent2.timeConstraint.customStart.date().day(), 5);

    // "2025.12.5" — dot format
    ParsedIntent intent3;
    m_parser->parse(QStringLiteral("2025.12.5的图片"), intent3);
    QCOMPARE(intent3.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent3.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent3.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent3.timeConstraint.customStart.date().day(), 5);
}

void tst_ChineseNLP::timeCustom_yesterday_variants_all()
{
    // "昨天下午" and "昨天晚上" — these are multi-char variants
    ParsedIntent intent1;
    m_parser->parse(QStringLiteral("昨天下午的图片"), intent1);
    QCOMPARE(intent1.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent1.timeConstraint.preset, TimePreset::Yesterday);

    ParsedIntent intent2;
    m_parser->parse(QStringLiteral("昨天晚上的视频"), intent2);
    QCOMPARE(intent2.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent2.timeConstraint.preset, TimePreset::Yesterday);
}

void tst_ChineseNLP::timeCustom_lastYear_extra()
{
    // "去年一整年" — not in current rules, but in requirements
    // Current rules only have "去年|上一年". Test that "去年" works.
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("去年的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::LastYear);
}

// ===== Filetype All-Synonyms Tests (from requirements) =====

void tst_ChineseNLP::fileType_document_general_allSynonyms()
{
    // Requirements 2.3.2.2.2: 文档, 文件, 报告, 文章, 方案, 文本, 资料, 笔记, 稿件
    const QStringList inputs = {
        QStringLiteral("文档"), QStringLiteral("文件"), QStringLiteral("报告"),
        QStringLiteral("文章"), QStringLiteral("方案"), QStringLiteral("文本"),
        QStringLiteral("资料"), QStringLiteral("笔记"), QStringLiteral("稿件")
    };
    const QStringList expectedExts = {"doc", "docx", "pdf", "txt", "wps", "rtf", "md", "odt"};
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(setEquals(intent.fileExtensions, expectedExts),
                 qPrintable(QStringLiteral("Failed for input: ") + input
                            + QStringLiteral(" got: ") + intent.fileExtensions.join(",")));
    }
}

void tst_ChineseNLP::fileType_spreadsheet_general_allSynonyms()
{
    // Requirements: 表格, 统计表, 报表, 名单, 数据表, 数据, 明细
    // NOTE: "数据" is excluded from rules due to high false-positive risk
    const QStringList inputs = {
        QStringLiteral("表格"), QStringLiteral("统计表"), QStringLiteral("报表"),
        QStringLiteral("名单"), QStringLiteral("数据表"), QStringLiteral("明细")
    };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(intent.fileExtensions.contains("xls"),
                 qPrintable(QStringLiteral("Missing xls for: ") + input));
        QVERIFY2(intent.fileExtensions.contains("xlsx"),
                 qPrintable(QStringLiteral("Missing xlsx for: ") + input));
        QVERIFY2(intent.fileExtensions.contains("csv"),
                 qPrintable(QStringLiteral("Missing csv for: ") + input));
    }
}

void tst_ChineseNLP::filetype_presentation_general_allSynonyms()
{
    // Requirements: 幻灯片, 演示文稿, 汇报, 课件, 宣讲
    const QStringList inputs = {
        QStringLiteral("幻灯片"), QStringLiteral("演示文稿"), QStringLiteral("汇报"),
        QStringLiteral("课件"), QStringLiteral("宣讲")
    };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(intent.fileExtensions.contains("ppt"),
                 qPrintable(QStringLiteral("Missing ppt for: ") + input));
        QVERIFY2(intent.fileExtensions.contains("pptx"),
                 qPrintable(QStringLiteral("Missing pptx for: ") + input));
    }
}

void tst_ChineseNLP::fileType_image_allSynonyms()
{
    // Requirements: 图片, 照片, 截图, 图, 壁纸, 海报, 相片, 表情包
    const QStringList inputs = {
        QStringLiteral("图片"), QStringLiteral("照片"), QStringLiteral("截图"),
        QStringLiteral("图"), QStringLiteral("壁纸"), QStringLiteral("海报"),
        QStringLiteral("相片"), QStringLiteral("表情包")
    };
    const QStringList expectedExts = {"jpg", "jpeg", "png", "gif", "bmp", "webp", "svg"};
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(setEquals(intent.fileExtensions, expectedExts),
                 qPrintable(QStringLiteral("Failed for input: ") + input));
    }
}

void tst_ChineseNLP::fileType_video_allSynonyms()
{
    // Requirements: 视频, 录像, 电影, 动画, 短片, 片子
    const QStringList inputs = {
        QStringLiteral("视频"), QStringLiteral("录像"), QStringLiteral("电影"),
        QStringLiteral("动画"), QStringLiteral("短片"), QStringLiteral("片子")
    };
    const QStringList expectedExts = {"mp4", "avi", "mkv", "mov", "flv", "wmv", "webm"};
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(setEquals(intent.fileExtensions, expectedExts),
                 qPrintable(QStringLiteral("Failed for input: ") + input));
    }
}

void tst_ChineseNLP::fileType_audio_allSynonyms()
{
    // Requirements: 音频, 音乐, 录音, 歌, 语音
    const QStringList inputs = {
        QStringLiteral("音频"), QStringLiteral("音乐"), QStringLiteral("录音"),
        QStringLiteral("歌"), QStringLiteral("语音")
    };
    const QStringList expectedExts = {"mp3", "wav", "flac", "aac", "ogg", "m4a"};
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(setEquals(intent.fileExtensions, expectedExts),
                 qPrintable(QStringLiteral("Failed for input: ") + input));
    }
}

void tst_ChineseNLP::fileType_archive_allSynonyms()
{
    // Requirements: 压缩包, 归档, 源码包, 打包文件, zip, rar
    const QStringList inputs = {
        QStringLiteral("压缩包"), QStringLiteral("归档"), QStringLiteral("源码包"),
        QStringLiteral("打包文件"), QStringLiteral("zip"), QStringLiteral("rar")
    };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(intent.fileExtensions.contains("zip"),
                 qPrintable(QStringLiteral("Missing zip for: ") + input));
    }
}

void tst_ChineseNLP::fileType_application_allSynonyms()
{
    // Requirements: 安装包, 软件, 应用, 脚本, 程序, 包
    // NOTE: "包" excluded from rules to avoid false positives with "表情包", "压缩包"
    const QStringList inputs = {
        QStringLiteral("安装包"), QStringLiteral("软件"), QStringLiteral("应用"),
        QStringLiteral("脚本"), QStringLiteral("程序")
    };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(intent.fileExtensions.contains("deb"),
                 qPrintable(QStringLiteral("Missing deb for: ") + input));
        QVERIFY2(intent.fileExtensions.contains("sh"),
                 qPrintable(QStringLiteral("Missing sh for: ") + input));
    }
}

void tst_ChineseNLP::fileType_design_source_allSynonyms()
{
    // Requirements: 源文件, 设计稿, psd, 矢量图, 工程文件
    const QStringList inputs = {
        QStringLiteral("源文件"), QStringLiteral("设计稿"), QStringLiteral("矢量图"),
        QStringLiteral("工程文件"), QStringLiteral("psd"), QStringLiteral("fig"),
        QStringLiteral("sketch")
    };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QVERIFY2(intent.fileExtensions.contains("psd"),
                 qPrintable(QStringLiteral("Missing psd for: ") + input));
        QVERIFY2(intent.fileExtensions.contains("ai"),
                 qPrintable(QStringLiteral("Missing ai for: ") + input));
    }
}

// ===== Combined Time+Type Tests =====

void tst_ChineseNLP::combined_fullDateAndType()
{
    // Requirements example: "2025年12月30日的文档"
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("2025年12月30日的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 12);
    QCOMPARE(intent.timeConstraint.customStart.date().day(), 30);
    // "文档" matches filetype_document_general
    QVERIFY(intent.fileExtensions.contains("doc"));
    QVERIFY(intent.fileExtensions.contains("pdf"));
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::combined_monthAndType()
{
    // "12月的图片" — month + image
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("12月的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().month(), 12);
    QVERIFY(intent.fileExtensions.contains("jpg"));
    QVERIFY(intent.fileExtensions.contains("png"));
    QVERIFY(intent.keywords.isEmpty());
}

void tst_ChineseNLP::combined_yearAndType()
{
    // "2025年的视频" — year + video
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("2025年的视频"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Custom);
    QCOMPARE(intent.timeConstraint.customStart.date().year(), 2025);
    QCOMPARE(intent.timeConstraint.customEnd.date().year(), 2025);
    QVERIFY(intent.fileExtensions.contains("mp4"));
    QVERIFY(intent.fileExtensions.contains("avi"));
}

// ===== Size Tests =====

void tst_ChineseNLP::size_fuzzy_large()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("大文件"), intent);
    QVERIFY(intent.sizeConstraint.isValid());
    QCOMPARE(intent.sizeConstraint.minSize, 524288000LL);  // 500MB
    QCOMPARE(intent.sizeConstraint.maxSize, 0LL);  // no upper bound
}

void tst_ChineseNLP::size_fuzzy_large_synonyms()
{
    const QStringList inputs = { QStringLiteral("很大的"), QStringLiteral("占空间的"),
                                  QStringLiteral("几个G的") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input + QStringLiteral("的图片"), intent);
        QVERIFY2(intent.sizeConstraint.isValid(),
                 qPrintable(QStringLiteral("Size not valid for: ") + input));
    }
}

void tst_ChineseNLP::size_fuzzy_small()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("小文件"), intent);
    QVERIFY(intent.sizeConstraint.isValid());
    QCOMPARE(intent.sizeConstraint.minSize, 0LL);
    QCOMPARE(intent.sizeConstraint.maxSize, 1048576LL);  // 1MB
    QCOMPARE(intent.sizeConstraint.includeUpper, false);
}

void tst_ChineseNLP::size_dynamic_min()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("大于500M的文档"), intent);
    QVERIFY(intent.sizeConstraint.isValid());
    QCOMPARE(intent.sizeConstraint.minSize, 524288000LL);  // 500MB
    QVERIFY(intent.sizeConstraint.includeLower);
}

void tst_ChineseNLP::size_dynamic_max()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("小于100K的文件"), intent);
    QVERIFY(intent.sizeConstraint.isValid());
    QCOMPARE(intent.sizeConstraint.maxSize, 102400LL);  // 100KB
    QCOMPARE(intent.sizeConstraint.minSize, 0LL);
}

void tst_ChineseNLP::size_dynamic_between()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("1M-10M的文件"), intent);
    QVERIFY(intent.sizeConstraint.isValid());
    QCOMPARE(intent.sizeConstraint.minSize, 1048576LL);   // 1MB
    QCOMPARE(intent.sizeConstraint.maxSize, 10485760LL);  // 10MB
}

void tst_ChineseNLP::size_combined_withTime()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天的大文件"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    QVERIFY(intent.sizeConstraint.isValid());
    QCOMPARE(intent.sizeConstraint.minSize, 524288000LL);
}

void tst_ChineseNLP::size_combined_withType()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("大文件 pdf"), intent);
    QVERIFY(intent.sizeConstraint.isValid());
    QVERIFY(intent.fileExtensions.contains("pdf"));
}

void tst_ChineseNLP::size_combined_full()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("昨天大于100M的图片和视频"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Yesterday);
    QVERIFY(intent.sizeConstraint.isValid());
    QCOMPARE(intent.sizeConstraint.minSize, 104857600LL);  // 100MB
    QVERIFY(intent.fileExtensions.contains("jpg"));
    QVERIFY(intent.fileExtensions.contains("mp4"));
}

// ===== Relative Time Tests =====

void tst_ChineseNLP::timeRelative_justNow()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("刚刚的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    // End should be very close to NOW (within 2 seconds)
    const qint64 endDelta = qAbs(intent.timeConstraint.customEnd.secsTo(QDateTime::currentDateTime()));
    QVERIFY2(endDelta < 2, "Relative end should be close to NOW");
    // Start should be ~2 hours ago
    const qint64 startDelta = qAbs(intent.timeConstraint.customStart.secsTo(QDateTime::currentDateTime().addSecs(-7200)));
    QVERIFY2(startDelta < 2, "Relative start should be ~2h ago");
    QVERIFY(intent.fileExtensions.contains("jpg"));
}

void tst_ChineseNLP::timeRelative_justNow_synonyms()
{
    const QStringList inputs = { QStringLiteral("刚才"), QStringLiteral("刚"),
                                  QStringLiteral("这会儿") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input + QStringLiteral("的文档"), intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    }
}

void tst_ChineseNLP::timeRelative_recentDays()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("最近的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    // End = NOW, Start = NOW - 3 days
    const qint64 endDelta = qAbs(intent.timeConstraint.customEnd.secsTo(QDateTime::currentDateTime()));
    QVERIFY2(endDelta < 2, "Recent days end should be NOW");
    const qint64 startDelta = qAbs(intent.timeConstraint.customStart.secsTo(QDateTime::currentDateTime().addSecs(-259200)));
    QVERIFY2(startDelta < 2, "Recent days start should be ~3 days ago");
}

void tst_ChineseNLP::timeRelative_recentDays_synonyms()
{
    const QStringList inputs = { QStringLiteral("这几天"), QStringLiteral("近期"),
                                  QStringLiteral("这阵子") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input + QStringLiteral("的文件"), intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    }
}

void tst_ChineseNLP::timeRelative_pastFewDays()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("前几天的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    // End = NOW - 3 days, Start = NOW - 7 days
    const qint64 endDelta = qAbs(intent.timeConstraint.customEnd.secsTo(QDateTime::currentDateTime().addSecs(-259200)));
    QVERIFY2(endDelta < 2, "Past few days end should be ~3 days ago");
    const qint64 startDelta = qAbs(intent.timeConstraint.customStart.secsTo(QDateTime::currentDateTime().addSecs(-604800)));
    QVERIFY2(startDelta < 2, "Past few days start should be ~7 days ago");
}

void tst_ChineseNLP::timeRelative_pastFewDays_synonyms()
{
    const QStringList inputs = { QStringLiteral("之前几天"), QStringLiteral("那些天") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input + QStringLiteral("的图片"), intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    }
}

void tst_ChineseNLP::timeRelative_aWhileAgo()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("之前的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    // End = NOW - 30 days
    const qint64 endDelta = qAbs(intent.timeConstraint.customEnd.secsTo(QDateTime::currentDateTime().addSecs(-2592000)));
    QVERIFY2(endDelta < 2, "A while ago end should be ~30 days ago");
    // Start should be epoch
    QCOMPARE(intent.timeConstraint.customStart, QDateTime::fromMSecsSinceEpoch(0));
}

void tst_ChineseNLP::timeRelative_aWhileAgo_synonyms()
{
    const QStringList inputs = { QStringLiteral("早些时候"), QStringLiteral("以前") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input + QStringLiteral("的文件"), intent);
        QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Relative);
    }
}

void tst_ChineseNLP::timeRelative_priority_vs_preset()
{
    // When both preset and relative could match, preset should win (higher priority)
    // "今天之前" — "今天" matches time_today (priority 200), "之前" matches time_a_while_ago (priority 80)
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天之前的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
}

// ===== Action Behavior Tests =====

void tst_ChineseNLP::action_create_birthTime()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("新建的图片"), intent);
    QCOMPARE(intent.timeConstraint.timeField, TimeField::BirthTime);
    // Action word should be consumed
    bool actionConsumed = false;
    for (const MatchSpan &span : intent.consumedSpans) {
        if (span.ruleId == "action_create") {
            actionConsumed = true;
            break;
        }
    }
    QVERIFY2(actionConsumed, "action_create should produce a consumed span");
}

void tst_ChineseNLP::action_create_synonyms()
{
    const QStringList inputs = { QStringLiteral("创建的文档"), QStringLiteral("存下来的图片"),
                                  QStringLiteral("保存的文件"), QStringLiteral("新加的视频") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.timeField, TimeField::BirthTime);
    }
}

void tst_ChineseNLP::action_modify_modifyTime()
{
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("修改过的图片"), intent);
    QCOMPARE(intent.timeConstraint.timeField, TimeField::ModifyTime);
    // Action word should be consumed
    bool actionConsumed = false;
    for (const MatchSpan &span : intent.consumedSpans) {
        if (span.ruleId == "action_modify") {
            actionConsumed = true;
            break;
        }
    }
    QVERIFY2(actionConsumed, "action_modify should produce a consumed span");
}

void tst_ChineseNLP::action_modify_synonyms()
{
    const QStringList inputs = { QStringLiteral("编辑过的文档"), QStringLiteral("改过的文件"),
                                  QStringLiteral("写过的图片"), QStringLiteral("更新的视频") };
    for (const QString &input : inputs) {
        ParsedIntent intent;
        m_parser->parse(input, intent);
        QCOMPARE(intent.timeConstraint.timeField, TimeField::ModifyTime);
    }
}

void tst_ChineseNLP::action_default_unspecified()
{
    // Without action words, timeField should remain Unspecified
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("今天的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    QCOMPARE(intent.timeConstraint.timeField, TimeField::Unspecified);
}

void tst_ChineseNLP::action_combined_withTime_create()
{
    // "新建的今天的文档" — action_create + time today
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("新建的今天的文档"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Today);
    QCOMPARE(intent.timeConstraint.timeField, TimeField::BirthTime);
    // Both time and action spans consumed
    int consumedCount = intent.consumedSpans.size();
    QVERIFY2(consumedCount >= 2,
             qPrintable(QStringLiteral("Expected >=2 consumed spans, got ") + QString::number(consumedCount)));
}

void tst_ChineseNLP::action_combined_withTime_modify()
{
    // "昨天修改过的图片" — time yesterday + action_modify
    ParsedIntent intent;
    m_parser->parse(QStringLiteral("昨天修改过的图片"), intent);
    QCOMPARE(intent.timeConstraint.kind, TimeConstraintKind::Preset);
    QCOMPARE(intent.timeConstraint.preset, TimePreset::Yesterday);
    QCOMPARE(intent.timeConstraint.timeField, TimeField::ModifyTime);
    QVERIFY(intent.fileExtensions.contains("jpg"));
}

QObject *create_tst_ChineseNLP()
{
    return new tst_ChineseNLP();
}

#include "tst_chinese_nlp.moc"
