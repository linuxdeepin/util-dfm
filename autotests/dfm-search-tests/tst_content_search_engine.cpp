// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <stubext.h>

#include <dfm-search/contentsearchapi.h>
#include <dfm-search/dsearch_global.h>
#include <dfm-search/field_names.h>
#include <dfm-search/ocrtextsearchapi.h>
#include <dfm-search/searchengine.h>
#include <dfm-search/textsearchapi.h>

#include <lucene++/Document.h>
#include <lucene++/FSDirectory.h>
#include <lucene++/Field.h>
#include <lucene++/IndexWriter.h>
#include <lucene++/LuceneHeaders.h>
#include <lucene++/ChineseAnalyzer.h>

using namespace DFMSEARCH;
using namespace Lucene;

namespace {

struct TestDocument
{
    QString path;
    QString filename;
    QString content;
    QString ancestorPath;
    QString hidden = "N";
    qint64 modifyTime = 1710000000;
    qint64 birthTime = 1700000000;
    qint64 fileSize = 1024;
};

DocumentPtr buildDocument(const TestDocument &docData)
{
    DocumentPtr doc = newLucene<Document>();
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kPath, docData.path.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kFilename, docData.filename.toStdWString(),
                              Field::STORE_YES, Field::INDEX_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kContents, docData.content.toStdWString(),
                              Field::STORE_YES, Field::INDEX_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kAncestorPaths, docData.ancestorPath.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kIsHidden, docData.hidden.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kModifyTime, QString::number(docData.modifyTime).toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kBirthTime, QString::number(docData.birthTime).toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::Content::kFileSize, QString::number(docData.fileSize).toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    return doc;
}

DocumentPtr buildOcrDocument(const TestDocument &docData)
{
    DocumentPtr doc = newLucene<Document>();
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kPath, docData.path.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kFilename, docData.filename.toStdWString(),
                              Field::STORE_YES, Field::INDEX_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kOcrContents, docData.content.toStdWString(),
                              Field::STORE_YES, Field::INDEX_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kAncestorPaths, docData.ancestorPath.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kIsHidden, docData.hidden.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kModifyTime, QString::number(docData.modifyTime).toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kBirthTime, QString::number(docData.birthTime).toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kFileSize, QString::number(docData.fileSize).toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::OcrText::kCheckSum,
                              QStringLiteral("checksum-%1").arg(docData.filename).toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    return doc;
}

void createContentIndex(const QString &indexDir, const QList<TestDocument> &documents)
{
    QDir().mkpath(indexDir);

    IndexWriterPtr writer = newLucene<IndexWriter>(
            FSDirectory::open(indexDir.toStdWString()),
            newLucene<ChineseAnalyzer>(),
            true,
            IndexWriter::MaxFieldLengthLIMITED);

    for (const TestDocument &doc : documents) {
        writer->addDocument(buildDocument(doc));
    }

    writer->close();
}

void createOcrIndex(const QString &indexDir, const QList<TestDocument> &documents)
{
    QDir().mkpath(indexDir);

    IndexWriterPtr writer = newLucene<IndexWriter>(
            FSDirectory::open(indexDir.toStdWString()),
            newLucene<ChineseAnalyzer>(),
            true,
            IndexWriter::MaxFieldLengthLIMITED);

    for (const TestDocument &doc : documents) {
        writer->addDocument(buildOcrDocument(doc));
    }

    writer->close();
}

SearchOptions createBaseOptions(const QString &searchPath, const QString &indexDir)
{
    (void) indexDir;
    SearchOptions options;
    options.setSearchMethod(SearchMethod::Indexed);
    options.setSearchPath(searchPath);
    options.setSyncSearchTimeout(5);
    return options;
}

QStringList resultPaths(const SearchResultExpected &expected)
{
    QStringList paths;
    const SearchResultList results = expected.value();
    for (const SearchResult &result : results) {
        paths.append(result.path());
    }
    return paths;
}

}   // namespace

class tst_ContentSearchEngine : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void search_simpleContent_usesTemporaryIndex();
    void search_booleanAnd_matchesContentOnly();
    void search_booleanOr_matchesAnyContent();
    void search_filenameAndContentMixed_requiresBoth();
    void search_mixedAnd_excludesPureFilenameOnlyMatches();
    void search_simpleOcr_usesTemporaryIndex();
    void search_ocrBooleanAnd_matchesOcrContentOnly();
    void search_ocrBooleanOr_matchesAnyOcrContent();
    void search_ocrFilenameAndContentMixed_requiresBoth();
    void search_ocrMixedAnd_excludesPureFilenameOnlyMatches();
};

void tst_ContentSearchEngine::search_simpleContent_usesTemporaryIndex()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/content-index";
    QVERIFY(QDir().mkpath(rootDir));

    createContentIndex(indexDir, {
                                     { rootDir + "/alpha-report.txt", "alpha-report.txt", "alpha budget summary", rootDir },
                                     { rootDir + "/meeting-notes.txt", "meeting-notes.txt", "meeting notes and timeline", rootDir },
                             });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::contentIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Content));
    engine->setSearchOptions(createBaseOptions(rootDir, indexDir));

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("budget"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/alpha-report.txt" });
}

void tst_ContentSearchEngine::search_booleanAnd_matchesContentOnly()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/content-index";
    QVERIFY(QDir().mkpath(rootDir));

    createContentIndex(indexDir, {
                                     { rootDir + "/roadmap.txt", "roadmap.txt", "alpha budget roadmap", rootDir },
                                     { rootDir + "/budget-only.txt", "budget-only.txt", "budget only", rootDir },
                                     { rootDir + "/alpha-only.txt", "alpha-only.txt", "alpha only", rootDir },
                             });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::contentIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Content));
    engine->setSearchOptions(createBaseOptions(rootDir, indexDir));

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "alpha", "budget" }, SearchQuery::BooleanOperator::AND));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/roadmap.txt" });
}

void tst_ContentSearchEngine::search_booleanOr_matchesAnyContent()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/content-index";
    QVERIFY(QDir().mkpath(rootDir));

    createContentIndex(indexDir, {
                                     { rootDir + "/alpha.txt", "alpha.txt", "alpha planning", rootDir },
                                     { rootDir + "/budget.txt", "budget.txt", "budget tracking", rootDir },
                                     { rootDir + "/other.txt", "other.txt", "travel notes", rootDir },
                             });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::contentIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Content));
    engine->setSearchOptions(createBaseOptions(rootDir, indexDir));

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "alpha", "budget" }, SearchQuery::BooleanOperator::OR));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains(rootDir + "/alpha.txt"));
    QVERIFY(paths.contains(rootDir + "/budget.txt"));
}

void tst_ContentSearchEngine::search_filenameAndContentMixed_requiresBoth()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/content-index";
    QVERIFY(QDir().mkpath(rootDir));

    createContentIndex(indexDir, {
                                     { rootDir + "/budget-alpha.txt", "budget-alpha.txt", "alpha roadmap", rootDir },
                                     { rootDir + "/budget-gamma.txt", "budget-gamma.txt", "gamma roadmap", rootDir },
                                     { rootDir + "/alpha-only.txt", "alpha-only.txt", "alpha roadmap", rootDir },
                             });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::contentIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir, indexDir);
    ContentOptionsAPI contentOptions(options);
    contentOptions.setFilenameKeyword("budget");

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Content));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("alpha"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/budget-alpha.txt" });
}

void tst_ContentSearchEngine::search_mixedAnd_excludesPureFilenameOnlyMatches()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/content-index";
    QVERIFY(QDir().mkpath(rootDir));

    createContentIndex(indexDir, {
                                     { rootDir + "/alpha-budget.txt", "alpha-budget.txt", "general notes", rootDir },
                                     { rootDir + "/alpha-plan.txt", "alpha-plan.txt", "budget implementation details", rootDir },
                                     { rootDir + "/budget-plan.txt", "budget-plan.txt", "alpha implementation details", rootDir },
                                     { rootDir + "/alpha-budget-content.txt", "alpha-budget-content.txt", "alpha budget implementation", rootDir },
                             });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::contentIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir, indexDir);
    ContentOptionsAPI contentOptions(options);
    contentOptions.setFilenameContentMixedAndSearchEnabled(true);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Content));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "alpha", "budget" }, SearchQuery::BooleanOperator::AND));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 3);
    QVERIFY(!paths.contains(rootDir + "/alpha-budget.txt"));
    QVERIFY(paths.contains(rootDir + "/alpha-plan.txt"));
    QVERIFY(paths.contains(rootDir + "/budget-plan.txt"));
    QVERIFY(paths.contains(rootDir + "/alpha-budget-content.txt"));
}

void tst_ContentSearchEngine::search_simpleOcr_usesTemporaryIndex()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/ocr-docs";
    const QString indexDir = tempDir.path() + "/ocr-index";
    QVERIFY(QDir().mkpath(rootDir));

    createOcrIndex(indexDir, {
                                 { rootDir + "/scan-a.png", "scan-a.png", "invoice amount recognized", rootDir },
                                 { rootDir + "/scan-b.png", "scan-b.png", "meeting room whiteboard", rootDir },
                         });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::ocrTextIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Ocr));
    engine->setSearchOptions(createBaseOptions(rootDir, indexDir));

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("invoice"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/scan-a.png" });
}

void tst_ContentSearchEngine::search_ocrBooleanAnd_matchesOcrContentOnly()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/ocr-docs";
    const QString indexDir = tempDir.path() + "/ocr-index";
    QVERIFY(QDir().mkpath(rootDir));

    createOcrIndex(indexDir, {
                                 { rootDir + "/receipt.png", "receipt.png", "invoice amount total", rootDir },
                                 { rootDir + "/amount-only.png", "amount-only.png", "amount only", rootDir },
                                 { rootDir + "/invoice-only.png", "invoice-only.png", "invoice only", rootDir },
                         });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::ocrTextIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Ocr));
    engine->setSearchOptions(createBaseOptions(rootDir, indexDir));

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "invoice", "amount" }, SearchQuery::BooleanOperator::AND));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/receipt.png" });
}

void tst_ContentSearchEngine::search_ocrBooleanOr_matchesAnyOcrContent()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/ocr-docs";
    const QString indexDir = tempDir.path() + "/ocr-index";
    QVERIFY(QDir().mkpath(rootDir));

    createOcrIndex(indexDir, {
                                 { rootDir + "/invoice.png", "invoice.png", "invoice recognized", rootDir },
                                 { rootDir + "/budget.png", "budget.png", "budget recognized", rootDir },
                                 { rootDir + "/other.png", "other.png", "travel receipt", rootDir },
                         });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::ocrTextIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Ocr));
    engine->setSearchOptions(createBaseOptions(rootDir, indexDir));

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "invoice", "budget" }, SearchQuery::BooleanOperator::OR));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains(rootDir + "/invoice.png"));
    QVERIFY(paths.contains(rootDir + "/budget.png"));
}

void tst_ContentSearchEngine::search_ocrFilenameAndContentMixed_requiresBoth()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/ocr-docs";
    const QString indexDir = tempDir.path() + "/ocr-index";
    QVERIFY(QDir().mkpath(rootDir));

    createOcrIndex(indexDir, {
                                 { rootDir + "/budget-invoice.png", "budget-invoice.png", "invoice details", rootDir },
                                 { rootDir + "/budget-other.png", "budget-other.png", "other details", rootDir },
                                 { rootDir + "/invoice-only.png", "invoice-only.png", "invoice details", rootDir },
                         });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::ocrTextIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir, indexDir);
    OcrTextOptionsAPI ocrOptions(options);
    ocrOptions.setFilenameKeyword("budget");

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Ocr));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("invoice"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/budget-invoice.png" });
}

void tst_ContentSearchEngine::search_ocrMixedAnd_excludesPureFilenameOnlyMatches()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/ocr-docs";
    const QString indexDir = tempDir.path() + "/ocr-index";
    QVERIFY(QDir().mkpath(rootDir));

    createOcrIndex(indexDir, {
                                 { rootDir + "/invoice-budget.png", "invoice-budget.png", "generic text", rootDir },
                                 { rootDir + "/invoice-note.png", "invoice-note.png", "budget recognized", rootDir },
                                 { rootDir + "/budget-note.png", "budget-note.png", "invoice recognized", rootDir },
                                 { rootDir + "/invoice-budget-content.png", "invoice-budget-content.png", "invoice budget recognized", rootDir },
                         });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::ocrTextIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir, indexDir);
    OcrTextOptionsAPI ocrOptions(options);
    ocrOptions.setFilenameOcrContentMixedAndSearchEnabled(true);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::Ocr));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "invoice", "budget" }, SearchQuery::BooleanOperator::AND));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 3);
    QVERIFY(!paths.contains(rootDir + "/invoice-budget.png"));
    QVERIFY(paths.contains(rootDir + "/invoice-note.png"));
    QVERIFY(paths.contains(rootDir + "/budget-note.png"));
    QVERIFY(paths.contains(rootDir + "/invoice-budget-content.png"));
}

QObject *create_tst_ContentSearchEngine()
{
    return new tst_ContentSearchEngine();
}

#include "tst_content_search_engine.moc"
