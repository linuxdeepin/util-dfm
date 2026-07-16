// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

#include "cli_options.h"

using namespace dfmsearch;

class tst_CliOptions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFeaturesCount();
    void testFeaturesContent();
    void testFeaturesOrder();
    void testFeaturesJsonOutput();
    void testFeaturesTextOutput();
};

// --- supportedFeatures() integrity tests ---

void tst_CliOptions::testFeaturesCount()
{
    QStringList features = CliOptions::supportedFeatures();
    QCOMPARE(features.size(), 26);
}

void tst_CliOptions::testFeaturesContent()
{
    QStringList features = CliOptions::supportedFeatures();
    QStringList expected = {
        "filename", "content", "ocr", "recent", "semantic",
        "indexed", "realtime",
        "simple", "boolean", "wildcard",
        "preview", "pinyin", "pinyin-acronym", "case-sensitive",
        "include-hidden", "file-types", "file-extensions", "exclude",
        "time-filter", "size-filter", "max-results", "max-preview",
        "offset", "filename-in-content", "json-output", "verbose"
    };
    QCOMPARE(features, expected);
}

void tst_CliOptions::testFeaturesOrder()
{
    QStringList features = CliOptions::supportedFeatures();
    // Verify first and last items to ensure order is preserved
    QCOMPARE(features.first(), QStringLiteral("filename"));
    QCOMPARE(features.last(), QStringLiteral("verbose"));

    // Verify category boundaries
    QCOMPARE(features.at(4), QStringLiteral("semantic"));   // last search type
    QCOMPARE(features.at(6), QStringLiteral("realtime"));   // last search method
    QCOMPARE(features.at(9), QStringLiteral("wildcard"));  // last query type
    QCOMPARE(features.at(10), QStringLiteral("preview"));  // first functional feature
}

void tst_CliOptions::testFeaturesJsonOutput()
{
    // Replicate the JSON formatting logic from parse() to verify structure
    QStringList features = CliOptions::supportedFeatures();

    QJsonObject root;
    root["type"] = QStringLiteral("features");
    QJsonArray featuresArray;
    for (const QString &feature : features) {
        featuresArray.append(feature);
    }
    root["features"] = featuresArray;
    root["count"] = features.size();
    QJsonDocument doc(root);

    QVERIFY(doc.isObject());
    QJsonObject obj = doc.object();
    QCOMPARE(obj.value("type").toString(), QStringLiteral("features"));
    QCOMPARE(obj.value("count").toInt(), 26);

    QJsonArray arr = obj.value("features").toArray();
    QCOMPARE(arr.size(), 26);
    QCOMPARE(arr.at(0).toString(), QStringLiteral("filename"));
    QCOMPARE(arr.at(25).toString(), QStringLiteral("verbose"));
}

void tst_CliOptions::testFeaturesTextOutput()
{
    // Replicate the text output formatting (space-separated) from parse()
    QStringList features = CliOptions::supportedFeatures();
    QString textOutput = features.join(' ');

    // Should be a single line with 25 spaces separating 26 items
    QCOMPARE(textOutput.count(' '), 25);
    QVERIFY(!textOutput.contains('\n'));
    QVERIFY(textOutput.startsWith(QStringLiteral("filename")));
    QVERIFY(textOutput.endsWith(QStringLiteral("verbose")));
}

QObject *create_tst_CliOptions()
{
    return new tst_CliOptions();
}

#include "tst_cli_options.moc"
