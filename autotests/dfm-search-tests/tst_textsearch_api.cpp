// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>
#include <dfm-search/textsearchapi.h>
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/ocrtextsearchapi.h>

using namespace DFMSEARCH;

class tst_TextSearchAPI : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // TextSearchOptionsAPI tests
    void textSearchOptions_defaultValues();
    void textSearchOptions_setMaxPreviewLength();
    void textSearchOptions_setSearchResultHighlightEnabled();
    void textSearchOptions_setFullTextRetrievalEnabled();

    // TextSearchResultAPI tests
    void textSearchResult_highlightedContent();
    void textSearchResult_filename();
    void textSearchResult_isHidden();
    void textSearchResult_modifyTimestamp();
    void textSearchResult_birthTimestamp();

    // ContentOptionsAPI tests
    void contentOptions_inheritance();
    void contentOptions_setFilenameContentMixedAndSearchEnabled();

    // ContentResultAPI tests
    void contentResult_inheritance();

    // OcrTextOptionsAPI tests
    void ocrTextOptions_inheritance();
    void ocrTextOptions_defaultValues();
    void ocrTextOptions_setFilenameOcrContentMixedAndSearchEnabled();

    // OcrTextResultAPI tests
    void ocrTextResult_inheritance();
    void ocrTextResult_ocrContent();
};

void tst_TextSearchAPI::initTestCase()
{
}

void tst_TextSearchAPI::cleanupTestCase()
{
}

// ==================== TextSearchOptionsAPI Tests ====================

void tst_TextSearchAPI::textSearchOptions_defaultValues()
{
    SearchOptions options;
    TextSearchOptionsAPI api(options);

    // 默认值在构造函数中设置，但基类构造函数不设置默认值
    // 默认值由子类设置
    QCOMPARE(api.maxPreviewLength(), 0);
    QCOMPARE(api.isSearchResultHighlightEnabled(), false);
    QCOMPARE(api.isFullTextRetrievalEnabled(), false);
}

void tst_TextSearchAPI::textSearchOptions_setMaxPreviewLength()
{
    SearchOptions options;
    TextSearchOptionsAPI api(options);

    api.setMaxPreviewLength(100);
    QCOMPARE(api.maxPreviewLength(), 100);

    api.setMaxPreviewLength(500);
    QCOMPARE(api.maxPreviewLength(), 500);

    api.setMaxPreviewLength(0);
    QCOMPARE(api.maxPreviewLength(), 0);
}

void tst_TextSearchAPI::textSearchOptions_setSearchResultHighlightEnabled()
{
    SearchOptions options;
    TextSearchOptionsAPI api(options);

    api.setSearchResultHighlightEnabled(true);
    QCOMPARE(api.isSearchResultHighlightEnabled(), true);

    api.setSearchResultHighlightEnabled(false);
    QCOMPARE(api.isSearchResultHighlightEnabled(), false);
}

void tst_TextSearchAPI::textSearchOptions_setFullTextRetrievalEnabled()
{
    SearchOptions options;
    TextSearchOptionsAPI api(options);

    api.setFullTextRetrievalEnabled(true);
    QCOMPARE(api.isFullTextRetrievalEnabled(), true);

    api.setFullTextRetrievalEnabled(false);
    QCOMPARE(api.isFullTextRetrievalEnabled(), false);
}

// ==================== TextSearchResultAPI Tests ====================

void tst_TextSearchAPI::textSearchResult_highlightedContent()
{
    SearchResult result("/test/path");
    TextSearchResultAPI api(result);

    QVERIFY(api.highlightedContent().isEmpty());

    api.setHighlightedContent("test <b>highlighted</b> content");
    QCOMPARE(api.highlightedContent(), QString("test <b>highlighted</b> content"));

    api.setHighlightedContent("");
    QVERIFY(api.highlightedContent().isEmpty());
}

void tst_TextSearchAPI::textSearchResult_filename()
{
    SearchResult result("/test/path");
    TextSearchResultAPI api(result);

    QVERIFY(api.filename().isEmpty());

    api.setFilename("test.txt");
    QCOMPARE(api.filename(), QString("test.txt"));

    api.setFilename("");
    QVERIFY(api.filename().isEmpty());
}

void tst_TextSearchAPI::textSearchResult_isHidden()
{
    SearchResult result("/test/path");
    TextSearchResultAPI api(result);

    QCOMPARE(api.isHidden(), false);

    api.setIsHidden(true);
    QCOMPARE(api.isHidden(), true);

    api.setIsHidden(false);
    QCOMPARE(api.isHidden(), false);
}

void tst_TextSearchAPI::textSearchResult_modifyTimestamp()
{
    SearchResult result("/test/path");
    TextSearchResultAPI api(result);

    QCOMPARE(api.modifyTimestamp(), 0);

    api.setModifyTimestamp(1700000000);
    QCOMPARE(api.modifyTimestamp(), 1700000000);
    QVERIFY(!api.modifyTimeString().isEmpty());

    api.setModifyTimestamp(0);
    QCOMPARE(api.modifyTimestamp(), 0);
}

void tst_TextSearchAPI::textSearchResult_birthTimestamp()
{
    SearchResult result("/test/path");
    TextSearchResultAPI api(result);

    QCOMPARE(api.birthTimestamp(), 0);

    api.setBirthTimestamp(1600000000);
    QCOMPARE(api.birthTimestamp(), 1600000000);
    QVERIFY(!api.birthTimeString().isEmpty());

    api.setBirthTimestamp(0);
    QCOMPARE(api.birthTimestamp(), 0);
}

// ==================== ContentOptionsAPI Tests ====================

void tst_TextSearchAPI::contentOptions_inheritance()
{
    SearchOptions options;
    ContentOptionsAPI api(options);

    // 验证继承自 TextSearchOptionsAPI
    api.setMaxPreviewLength(300);
    QCOMPARE(api.maxPreviewLength(), 300);

    api.setSearchResultHighlightEnabled(true);
    QCOMPARE(api.isSearchResultHighlightEnabled(), true);

    api.setFullTextRetrievalEnabled(false);
    QCOMPARE(api.isFullTextRetrievalEnabled(), false);
}

void tst_TextSearchAPI::contentOptions_setFilenameContentMixedAndSearchEnabled()
{
    SearchOptions options;
    ContentOptionsAPI api(options);

    QCOMPARE(api.isFilenameContentMixedAndSearchEnabled(), false);

    api.setFilenameContentMixedAndSearchEnabled(true);
    QCOMPARE(api.isFilenameContentMixedAndSearchEnabled(), true);

    api.setFilenameContentMixedAndSearchEnabled(false);
    QCOMPARE(api.isFilenameContentMixedAndSearchEnabled(), false);
}

// ==================== ContentResultAPI Tests ====================

void tst_TextSearchAPI::contentResult_inheritance()
{
    SearchResult result("/test/path");
    ContentResultAPI api(result);

    // 验证继承自 TextSearchResultAPI
    api.setHighlightedContent("content match");
    QCOMPARE(api.highlightedContent(), QString("content match"));

    api.setFilename("document.pdf");
    QCOMPARE(api.filename(), QString("document.pdf"));

    api.setIsHidden(true);
    QCOMPARE(api.isHidden(), true);

    api.setModifyTimestamp(1700000000);
    QCOMPARE(api.modifyTimestamp(), 1700000000);

    api.setBirthTimestamp(1600000000);
    QCOMPARE(api.birthTimestamp(), 1600000000);
}

// ==================== OcrTextOptionsAPI Tests ====================

void tst_TextSearchAPI::ocrTextOptions_inheritance()
{
    SearchOptions options;
    OcrTextOptionsAPI api(options);

    // 验证继承自 TextSearchOptionsAPI
    api.setMaxPreviewLength(250);
    QCOMPARE(api.maxPreviewLength(), 250);

    api.setSearchResultHighlightEnabled(true);
    QCOMPARE(api.isSearchResultHighlightEnabled(), true);

    api.setFullTextRetrievalEnabled(false);
    QCOMPARE(api.isFullTextRetrievalEnabled(), false);
}

void tst_TextSearchAPI::ocrTextOptions_defaultValues()
{
    SearchOptions options;
    OcrTextOptionsAPI api(options);

    // 验证默认值
    QCOMPARE(api.maxPreviewLength(), 200);
    QCOMPARE(api.isSearchResultHighlightEnabled(), false);
    QCOMPARE(api.isFullTextRetrievalEnabled(), true);
    QCOMPARE(api.isFilenameOcrContentMixedAndSearchEnabled(), false);
}

void tst_TextSearchAPI::ocrTextOptions_setFilenameOcrContentMixedAndSearchEnabled()
{
    SearchOptions options;
    OcrTextOptionsAPI api(options);

    QCOMPARE(api.isFilenameOcrContentMixedAndSearchEnabled(), false);

    api.setFilenameOcrContentMixedAndSearchEnabled(true);
    QCOMPARE(api.isFilenameOcrContentMixedAndSearchEnabled(), true);

    api.setFilenameOcrContentMixedAndSearchEnabled(false);
    QCOMPARE(api.isFilenameOcrContentMixedAndSearchEnabled(), false);
}

// ==================== OcrTextResultAPI Tests ====================

void tst_TextSearchAPI::ocrTextResult_inheritance()
{
    SearchResult result("/test/path");
    OcrTextResultAPI api(result);

    // 验证继承自 TextSearchResultAPI
    api.setHighlightedContent("OCR match");
    QCOMPARE(api.highlightedContent(), QString("OCR match"));

    api.setFilename("image.png");
    QCOMPARE(api.filename(), QString("image.png"));

    api.setIsHidden(true);
    QCOMPARE(api.isHidden(), true);

    api.setModifyTimestamp(1700000000);
    QCOMPARE(api.modifyTimestamp(), 1700000000);

    api.setBirthTimestamp(1600000000);
    QCOMPARE(api.birthTimestamp(), 1600000000);
}

void tst_TextSearchAPI::ocrTextResult_ocrContent()
{
    SearchResult result("/test/path");
    OcrTextResultAPI api(result);

    QVERIFY(api.ocrContent().isEmpty());

    api.setOcrContent("This is extracted OCR text from image");
    QCOMPARE(api.ocrContent(), QString("This is extracted OCR text from image"));

    api.setOcrContent("");
    QVERIFY(api.ocrContent().isEmpty());
}

QObject *create_tst_TextSearchAPI()
{
    return new tst_TextSearchAPI();
}

#include "tst_textsearch_api.moc"
