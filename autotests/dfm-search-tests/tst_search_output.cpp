// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>

#include <cstdio>
#include <functional>
#include <iostream>
#include <unistd.h>

#include <dfm-search/contentretriever.h>
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/field_names.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/ocrtextsearchapi.h>
#include <dfm-search/searchoptions.h>

#include <lucene++/Document.h>
#include <lucene++/Field.h>
#include <lucene++/FSDirectory.h>
#include <lucene++/IndexWriter.h>
#include <lucene++/KeywordAnalyzer.h>
#include <lucene++/LuceneHeaders.h>

#include "output/json_output.h"
#include "output/text_output.h"
#include "preview_command.h"
#include "preview_output_utils.h"

using namespace DFMSEARCH;
using namespace Lucene;

namespace {

void addStoredDocument(const IndexWriterPtr &writer,
                       SearchType type,
                       const QString &path,
                       const QString &filename,
                       const QString &content)
{
    DocumentPtr doc = newLucene<Document>();
    if (type == SearchType::Ocr) {
        doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kPath, path.toStdWString(),
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
        doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kFilename, filename.toStdWString(),
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
        doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kOcrContents, content.toStdWString(),
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    } else {
        doc->add(newLucene<Field>(LuceneFieldNames::Content::kPath, path.toStdWString(),
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
        doc->add(newLucene<Field>(LuceneFieldNames::Content::kFilename, filename.toStdWString(),
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
        doc->add(newLucene<Field>(LuceneFieldNames::Content::kContents, content.toStdWString(),
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    }
    writer->addDocument(doc);
}

void createIndex(const QString &indexDir, SearchType type)
{
    QDir().mkpath(indexDir);
    IndexWriterPtr writer = newLucene<IndexWriter>(
            FSDirectory::open(indexDir.toStdWString()),
            newLucene<KeywordAnalyzer>(),
            true,
            IndexWriter::MaxFieldLengthLIMITED);

    if (type == SearchType::Content) {
        addStoredDocument(writer, type,
                          "/tmp/doc-a.txt",
                          "doc-a.txt",
                          "hello world from content index");
    } else {
        addStoredDocument(writer, type,
                          "/tmp/img-a.png",
                          "img-a.png",
                          "screenshot text from OCR");
    }

    writer->close();
}

QString captureStdout(const std::function<void()> &fn)
{
    QTemporaryFile file;
    if (!file.open()) {
        return {};
    }

    fflush(stdout);
    std::cout.flush();

    const int savedStdout = dup(STDOUT_FILENO);
    if (savedStdout < 0) {
        return {};
    }

    dup2(file.handle(), STDOUT_FILENO);
    fn();
    fflush(stdout);
    std::cout.flush();

    dup2(savedStdout, STDOUT_FILENO);
    close(savedStdout);

    file.seek(0);
    return QString::fromUtf8(file.readAll());
}

}   // namespace

class tst_SearchOutput : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void previewOutputHelpers_includeCharCount();
    void previewCommand_failsForUnindexedPath();
    void jsonOutput_contentAndFilenameCharCountContract();
    void jsonOutput_ocrAndSemanticCharCountContract();
    void textOutput_contentAndSemanticCharCountContract();
};

void tst_SearchOutput::previewOutputHelpers_includeCharCount()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);

    PreviewOptions options;
    options.setKeyword("world");
    options.setOffset(0);
    options.setMaxLength(10);

    const PreviewResult result = retriever.fetchPreview("/tmp/doc-a.txt", SearchType::Content, options);
    const QJsonObject json = dfmsearch::previewResultToJson("/tmp/doc-a.txt", result);
    QCOMPARE(json.value("content").toString(), QString("world from"));
    QCOMPARE(json.value("charCount").toInt(), QString("hello world from content index").size());
    QCOMPARE(json.value("keywordOffset").toInt(), 6);

    QString text;
    QTextStream stream(&text);
    dfmsearch::writePreviewResultText(stream, "/tmp/doc-a.txt", result);
    QVERIFY(text.contains("Char count: 30"));
    QVERIFY(text.contains("world from"));
}

void tst_SearchOutput::previewCommand_failsForUnindexedPath()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);

    SearchCliConfig config;
    config.subcommand = "preview";
    config.searchType = SearchType::Content;
    config.searchPath = "/tmp/doc-a.txt,/tmp/missing.txt";
    config.jsonOutput = true;

    const PreviewCommandResult result = dfmsearch::runPreviewCommand(config, retriever);
    QCOMPARE(result.exitCode, 1);
    QVERIFY(result.stdoutText.isEmpty());
    QVERIFY(result.stderrText.contains("Error: preview requires indexed content for:"));
    QVERIFY(result.stderrText.contains("/tmp/missing.txt"));
    QVERIFY(!result.stderrText.contains("/tmp/doc-a.txt"));
}

void tst_SearchOutput::jsonOutput_contentAndFilenameCharCountContract()
{
    SearchOptions options;
    options.setDetailedResultsEnabled(true);

    SearchResult contentResult("/tmp/doc-a.txt");
    ContentResultAPI contentApi(contentResult);
    contentApi.setCharCount(30);
    contentApi.setFilename("doc-a.txt");
    contentResult.setCustomAttribute("plainContentMatch", QString("world from"));
    contentResult.setCustomAttribute("snippetOffset", 6);

    dfmsearch::JsonOutput contentOutput(false);
    contentOutput.setSearchOptions(options);
    contentOutput.setSearchContext("world", "/tmp", SearchType::Content, SearchMethod::Indexed);
    const QString contentJsonText = captureStdout([&]() {
        contentOutput.outputSearchStarted();
        contentOutput.outputSearchFinished({ contentResult });
    });
    const QJsonObject contentRoot = QJsonDocument::fromJson(contentJsonText.toUtf8()).object();
    const QJsonObject contentItem = contentRoot.value("results").toArray().first().toObject();
    QCOMPARE(contentItem.value("charCount").toInt(), 30);

    SearchResult filenameResult("/tmp/doc-a.txt");
    FileNameResultAPI fileApi(filenameResult);
    fileApi.setFilename("doc-a.txt");
    fileApi.setFileType("txt");
    fileApi.setSize("123");
    filenameResult.setCustomAttribute("charCount", 30);

    dfmsearch::JsonOutput filenameOutput(false);
    filenameOutput.setSearchOptions(options);
    filenameOutput.setSearchContext("doc", "/tmp", SearchType::FileName, SearchMethod::Indexed);
    const QString filenameJsonText = captureStdout([&]() {
        filenameOutput.outputSearchStarted();
        filenameOutput.outputSearchFinished({ filenameResult });
    });
    const QJsonObject filenameRoot = QJsonDocument::fromJson(filenameJsonText.toUtf8()).object();
    const QJsonObject filenameItem = filenameRoot.value("results").toArray().first().toObject();
    QVERIFY(!filenameItem.contains("charCount"));
}

void tst_SearchOutput::jsonOutput_ocrAndSemanticCharCountContract()
{
    SearchOptions options;
    options.setDetailedResultsEnabled(true);

    SearchResult ocrResult("/tmp/img-a.png");
    OcrTextResultAPI ocrApi(ocrResult);
    ocrApi.setCharCount(24);
    ocrApi.setOcrContent("screenshot text from OCR");
    ocrResult.setCustomAttribute("plainContentMatch", QString("text from OCR"));
    ocrResult.setCustomAttribute("snippetOffset", 11);

    dfmsearch::JsonOutput ocrOutput(false);
    ocrOutput.setSearchOptions(options);
    ocrOutput.setSearchContext("text", "/tmp", SearchType::Ocr, SearchMethod::Indexed);
    const QString ocrJsonText = captureStdout([&]() {
        ocrOutput.outputSearchStarted();
        ocrOutput.outputSearchFinished({ ocrResult });
    });
    const QJsonObject ocrRoot = QJsonDocument::fromJson(ocrJsonText.toUtf8()).object();
    const QJsonObject ocrItem = ocrRoot.value("results").toArray().first().toObject();
    QCOMPARE(ocrItem.value("charCount").toInt(), 24);

    SearchResult semanticResult("/tmp/doc-a.txt");
    semanticResult.setCustomAttribute("charCount", 30);
    semanticResult.setCustomAttribute("contentMatch", QString("world from"));

    dfmsearch::JsonOutput semanticOutput(false);
    semanticOutput.setSearchOptions(options);
    semanticOutput.setSearchContext("world", "/tmp", SearchType::Semantic, SearchMethod::Indexed);
    const QString semanticJsonText = captureStdout([&]() {
        semanticOutput.outputSearchStarted();
        semanticOutput.outputSearchFinished({ semanticResult });
    });
    const QJsonObject semanticRoot = QJsonDocument::fromJson(semanticJsonText.toUtf8()).object();
    const QJsonObject semanticItem = semanticRoot.value("results").toArray().first().toObject();
    QCOMPARE(semanticItem.value("charCount").toInt(), 30);
}

void tst_SearchOutput::textOutput_contentAndSemanticCharCountContract()
{
    SearchOptions options;
    options.setDetailedResultsEnabled(true);

    SearchResult contentResult("/tmp/doc-a.txt");
    ContentResultAPI contentApi(contentResult);
    contentApi.setCharCount(30);
    contentResult.setCustomAttribute("plainContentMatch", QString("world from"));
    contentResult.setCustomAttribute("snippetOffset", 6);

    dfmsearch::TextOutput contentOutput;
    contentOutput.setSearchOptions(options);
    contentOutput.setVerbose(true);
    contentOutput.setSearchContext("world", "/tmp", SearchType::Content, SearchMethod::Indexed);
    const QString contentText = captureStdout([&]() {
        contentOutput.outputSearchFinished({ contentResult });
    });
    QVERIFY(contentText.contains("Char count: 30"));

    SearchResult semanticResult("/tmp/doc-a.txt");
    semanticResult.setCustomAttribute("charCount", 30);
    semanticResult.setCustomAttribute("contentMatch", QString("world from"));

    dfmsearch::TextOutput semanticOutput;
    semanticOutput.setSearchOptions(options);
    semanticOutput.setVerbose(true);
    semanticOutput.setSearchContext("world", "/tmp", SearchType::Semantic, SearchMethod::Indexed);
    const QString semanticText = captureStdout([&]() {
        semanticOutput.outputSearchFinished({ semanticResult });
    });
    QVERIFY(semanticText.contains("charCount: 30"));
}

QObject *create_tst_SearchOutput()
{
    return new tst_SearchOutput();
}

#include "tst_search_output.moc"
