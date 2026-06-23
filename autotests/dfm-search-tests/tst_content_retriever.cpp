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
    options.maxPreviewLength = 80;

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
    options.maxPreviewLength = 80;

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

QObject *create_tst_ContentRetriever()
{
    return new tst_ContentRetriever();
}

#include "tst_content_retriever.moc"
