// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <atomic>
#include <future>
#include <vector>

#include <dfm-search/contentretriever.h>
#include <dfm-search/field_names.h>

#include "utils/contenthighlighter.h"

#include <lucene++/Document.h>
#include <lucene++/Field.h>
#include <lucene++/FSDirectory.h>
#include <lucene++/IndexWriter.h>
#include <lucene++/KeywordAnalyzer.h>
#include <lucene++/LuceneHeaders.h>

using namespace dfmsearch;
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
        addStoredDocument(writer, type,
                          "/tmp/doc-b.txt",
                          "doc-b.txt",
                          "meeting notes and budget data");
    } else {
        addStoredDocument(writer, type,
                          "/tmp/img-a.png",
                          "img-a.png",
                          "screenshot text from OCR");
    }

    writer->close();
}

}   // namespace

class tst_ContentRetriever : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fetchContent_single();
    void fetchContent_batch();
    void fetchHighlight_usesTemporaryIndex();
    void fetchContent_semanticRoutingFailsWhenNoDConfig();
    void fetchHighlight_semanticRoutingFailsWhenNoDConfig();
    void concurrentFetch_sharedRetriever();
    void fetchPreview_noKeyword();
    void fetchPreview_withKeyword();
    void fetchPreview_keywordNotFound();
    void fetchPreview_offsetBeyondContent();
    void previewSnippet_basic();
};

void tst_ContentRetriever::fetchContent_single()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    const QString ocrIndexDir = tempDir.path() + "/ocr-index";
    createIndex(contentIndexDir, SearchType::Content);
    createIndex(ocrIndexDir, SearchType::Ocr);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);
    retriever.setIndexDirectory(SearchType::Ocr, ocrIndexDir);

    QCOMPARE(retriever.fetchContent("/tmp/doc-a.txt", SearchType::Content),
             QString("hello world from content index"));
    QCOMPARE(retriever.fetchContent("/tmp/img-a.png", SearchType::Ocr),
             QString("screenshot text from OCR"));
    QVERIFY(retriever.fetchContent("/tmp/missing.txt", SearchType::Content).isEmpty());
}

void tst_ContentRetriever::fetchContent_batch()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);
    const QMap<QString, QString> results = retriever.fetchContents(
            { "/tmp/doc-a.txt", "/tmp/doc-b.txt", "/tmp/missing.txt" },
            SearchType::Content);

    QCOMPARE(results.value("/tmp/doc-a.txt"), QString("hello world from content index"));
    QCOMPARE(results.value("/tmp/doc-b.txt"), QString("meeting notes and budget data"));
    QVERIFY(results.contains("/tmp/missing.txt"));
    QVERIFY(results.value("/tmp/missing.txt").isEmpty());
}

void tst_ContentRetriever::fetchHighlight_usesTemporaryIndex()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);
    HighlightOptions options;
    options.setMaxPreviewLength(80);

    const QString snippet = retriever.fetchHighlight("/tmp/doc-b.txt",
                                                     "budget",
                                                     SearchType::Content,
                                                     options);
    QVERIFY(snippet.contains("budget", Qt::CaseInsensitive));
}

void tst_ContentRetriever::fetchContent_semanticRoutingFailsWhenNoDConfig()
{
    // In test environment dconfig is unavailable, so semanticDocExtensions()/semanticPicExtensions()
    // return empty sets → Semantic type should be rejected with a warning and return empty.
    ContentRetriever retriever;
    QVERIFY(retriever.fetchContent("/tmp/doc-a.txt", SearchType::Semantic).isEmpty());
    QVERIFY(retriever.fetchContent("/tmp/img-a.png", SearchType::Semantic).isEmpty());
}

void tst_ContentRetriever::fetchHighlight_semanticRoutingFailsWhenNoDConfig()
{
    ContentRetriever retriever;
    HighlightOptions options;
    QVERIFY(retriever.fetchHighlight("/tmp/doc-b.txt", "budget", SearchType::Semantic, options).isEmpty());
}

void tst_ContentRetriever::concurrentFetch_sharedRetriever()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    const QString ocrIndexDir = tempDir.path() + "/ocr-index";
    createIndex(contentIndexDir, SearchType::Content);
    createIndex(ocrIndexDir, SearchType::Ocr);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);
    retriever.setIndexDirectory(SearchType::Ocr, ocrIndexDir);

    HighlightOptions options;
    options.setMaxPreviewLength(80);

    std::atomic_bool failed { false };
    std::vector<std::future<void>> tasks;
    tasks.reserve(8);

    for (int worker = 0; worker < 8; ++worker) {
        tasks.emplace_back(std::async(std::launch::async, [&retriever, &options, &failed]() {
            for (int i = 0; i < 50; ++i) {
                if (retriever.fetchContent("/tmp/doc-a.txt", SearchType::Content)
                    != QString("hello world from content index")) {
                    failed.store(true);
                    return;
                }

                if (retriever.fetchContent("/tmp/img-a.png", SearchType::Ocr)
                    != QString("screenshot text from OCR")) {
                    failed.store(true);
                    return;
                }

                const QString snippet = retriever.fetchHighlight("/tmp/doc-b.txt",
                                                                 "budget",
                                                                 SearchType::Content,
                                                                 options);
                if (!snippet.contains("budget", Qt::CaseInsensitive)) {
                    failed.store(true);
                    return;
                }
            }
        }));
    }

    for (auto &task : tasks) {
        task.get();
    }

    QVERIFY(!failed.load());
}

void tst_ContentRetriever::fetchPreview_noKeyword()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);

    PreviewOptions options;
    options.setKeyword(QString());
    options.setOffset(6);        // skip "hello "
    options.setMaxLength(5);    // expect "world"

    const PreviewResult result = retriever.fetchPreview("/tmp/doc-a.txt",
                                                         SearchType::Content,
                                                         options);
    QCOMPARE(result.content(), QString("world"));
    QCOMPARE(result.keywordOffset(), -1);
}

void tst_ContentRetriever::fetchPreview_withKeyword()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);

    // Content of doc-a.txt is "hello world from content index"
    // "world" starts at position 6
    PreviewOptions options;
    options.setKeyword("world");
    options.setOffset(0);
    options.setMaxLength(10);

    const PreviewResult result = retriever.fetchPreview("/tmp/doc-a.txt",
                                                         SearchType::Content,
                                                         options);
    QCOMPARE(result.content(), QString("world from"));
    QCOMPARE(result.keywordOffset(), 6);
}

void tst_ContentRetriever::fetchPreview_keywordNotFound()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);

    // Content of doc-a.txt is "hello world from content index" (30 chars)
    // offset=300 is beyond the content, so keyword won't be found
    PreviewOptions options;
    options.setKeyword("world");
    options.setOffset(300);
    options.setMaxLength(10);

    const PreviewResult result = retriever.fetchPreview("/tmp/doc-a.txt",
                                                         SearchType::Content,
                                                         options);
    QVERIFY(result.content().isEmpty());
    QCOMPARE(result.keywordOffset(), -1);
}

void tst_ContentRetriever::fetchPreview_offsetBeyondContent()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString contentIndexDir = tempDir.path() + "/content-index";
    createIndex(contentIndexDir, SearchType::Content);

    ContentRetriever retriever;
    retriever.setIndexDirectory(SearchType::Content, contentIndexDir);

    // No keyword, offset beyond content length → empty content
    PreviewOptions options;
    options.setKeyword(QString());
    options.setOffset(1000);
    options.setMaxLength(50);

    const PreviewResult result = retriever.fetchPreview("/tmp/doc-a.txt",
                                                         SearchType::Content,
                                                         options);
    QVERIFY(result.content().isEmpty());
    QCOMPARE(result.keywordOffset(), -1);
}

void tst_ContentRetriever::previewSnippet_basic()
{
    using DFMSEARCH::ContentHighlighter::previewSnippet;

    const QString content = QStringLiteral("hello world from content index");

    // No keyword: simple offset + maxLength slice
    int kwOff = 42;
    QCOMPARE(previewSnippet(content, 6, 5, QString(), &kwOff), QString("world"));
    QCOMPARE(kwOff, -1);

    // With keyword: search from offset, return from match position
    QCOMPARE(previewSnippet(content, 0, 10, "world", &kwOff), QString("world from"));
    QCOMPARE(kwOff, 6);

    // Keyword not found
    QVERIFY(previewSnippet(content, 0, 10, "nonexistent", &kwOff).isEmpty());
    QCOMPARE(kwOff, -1);

    // Offset beyond content
    QVERIFY(previewSnippet(content, 1000, 10, QString(), &kwOff).isEmpty());

    // Negative offset → empty
    QVERIFY(previewSnippet(content, -1, 10, QString(), &kwOff).isEmpty());
}

QObject *create_tst_ContentRetriever()
{
    return new tst_ContentRetriever();
}

#include "tst_content_retriever.moc"
