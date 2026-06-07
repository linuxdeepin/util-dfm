// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <stubext.h>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/field_names.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/searchengine.h>
#include <dfm-search/searcherror.h>

#include <lucene++/Document.h>
#include <lucene++/FSDirectory.h>
#include <lucene++/Field.h>
#include <lucene++/IndexWriter.h>
#include <lucene++/LuceneHeaders.h>
#include <lucene++/ChineseAnalyzer.h>
#include <lucene++/NumericField.h>

using namespace DFMSEARCH;
using namespace Lucene;

namespace {

struct TestDocument
{
    QString path;
    QString filename;
    QString fileType;
    QString fileExt;
    QString pinyin;
    QString pinyinAcronym;
    QString hidden = "N";
    qint64 modifyTime = 1710000000;
    qint64 birthTime = 1700000000;
    qint64 fileSize = 1024;
    QString fileSizeStr = "1 KB";
};

QStringList ancestorPathsForDocument(const QString &path)
{
    QStringList ancestors;
    QFileInfo info(path);
    QDir dir = info.dir();

    while (dir.exists()) {
        const QString current = QDir::cleanPath(dir.absolutePath());
        ancestors.append(current);
        if (!dir.cdUp()) {
            break;
        }
    }

    ancestors.removeDuplicates();
    return ancestors;
}

DocumentPtr buildDocument(const TestDocument &docData)
{
    DocumentPtr doc = newLucene<Document>();

    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kFullPath, docData.path.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kFileName, docData.filename.toStdWString(),
                              Field::STORE_YES, Field::INDEX_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kFileNameLower, docData.filename.toLower().toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kFileType, docData.fileType.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kFileExt, docData.fileExt.toLower().toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kPinyin, docData.pinyin.toStdWString(),
                              Field::STORE_YES, Field::INDEX_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kPinyinAcronym, docData.pinyinAcronym.toStdWString(),
                              Field::STORE_YES, Field::INDEX_ANALYZED));
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kIsHidden, docData.hidden.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    NumericFieldPtr modifyTimeField = newLucene<NumericField>(LuceneFieldNames::FileName::kModifyTime,
                                                              Field::STORE_YES, true);
    modifyTimeField->setLongValue(docData.modifyTime);
    doc->add(modifyTimeField);

    NumericFieldPtr birthTimeField = newLucene<NumericField>(LuceneFieldNames::FileName::kBirthTime,
                                                             Field::STORE_YES, true);
    birthTimeField->setLongValue(docData.birthTime);
    doc->add(birthTimeField);

    NumericFieldPtr fileSizeField = newLucene<NumericField>(LuceneFieldNames::FileName::kFileSize,
                                                            Field::STORE_YES, true);
    fileSizeField->setLongValue(docData.fileSize);
    doc->add(fileSizeField);
    doc->add(newLucene<Field>(LuceneFieldNames::FileName::kFileSizeStr, docData.fileSizeStr.toStdWString(),
                              Field::STORE_YES, Field::INDEX_NOT_ANALYZED));

    for (const QString &ancestor : ancestorPathsForDocument(docData.path)) {
        doc->add(newLucene<Field>(LuceneFieldNames::FileName::kAncestorPaths, ancestor.toStdWString(),
                                  Field::STORE_YES, Field::INDEX_NOT_ANALYZED));
    }

    return doc;
}

void createFileNameIndex(const QString &indexDir, const QList<TestDocument> &documents)
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

SearchOptions createBaseOptions(const QString &searchPath)
{
    SearchOptions options;
    options.setSearchMethod(SearchMethod::Indexed);
    options.setSearchPath(searchPath);
    options.setSyncSearchTimeout(5);
    return options;
}

SearchOptions createRealtimeOptions(const QString &searchPath)
{
    SearchOptions options = createBaseOptions(searchPath);
    options.setSearchMethod(SearchMethod::Realtime);
    return options;
}

bool createFileWithSize(const QString &path, qint64 size)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (size > 0) {
        file.write(QByteArray(static_cast<int>(size), 'a'));
    }

    file.close();
    return true;
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

SearchQuery createWildcardQuery(const QString &pattern)
{
    SearchQuery query(pattern, SearchQuery::Type::Wildcard);
    return query;
}

}   // namespace

class tst_FileNameSearchEngine : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void search_simpleKeyword_matchesIndexedFilename();
    void search_booleanAnd_requiresAllTerms();
    void search_booleanOr_matchesAnyTerm();
    void search_wildcard_matchesByPattern();
    void search_fileTypeFilterOnly_returnsAllMatchingTypes();
    void search_extensionFilterOnly_returnsAllMatchingSuffixes();
    void search_keywordAndTypeFilter_requiresBoth();
    void search_keywordAndExtensionFilter_requiresBoth();
    void search_hiddenFiles_excludedByDefault_andIncludedWhenEnabled();
    void search_excludedPath_filtersSubtreeAtQueryLayer();
    void search_sizeAndTimeFilters_applyOnIndexedFields();
    void search_pinyinAndAcronym_queriesMatchIndexedFields();
    void search_detailedResults_populatesExtendedAttributes();
    void search_emptyKeywordWithoutFilters_returnsValidationError();
    void search_invalidFileType_returnsValidationError();
    void realtime_simpleKeyword_matchesFilesystemEntries();
    void realtime_booleanAndOr_andWildcard_queriesWork();
    void realtime_extensionFilters_areApplied();
    void realtime_hiddenAndExcludedPath_filtersWork();
    void realtime_sizeAndTimeFilters_applyWithoutIndex();
    void realtime_detailedResults_populateAttributes();
    void realtime_pinyinOption_doesNotProducePinyinMatches();
};

void tst_FileNameSearchEngine::search_simpleKeyword_matchesIndexedFilename()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/alpha-report.txt", "alpha-report.txt", "doc", "txt" },
                                      { rootDir + "/meeting-notes.txt", "meeting-notes.txt", "doc", "txt" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(createBaseOptions(rootDir));

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("report"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/alpha-report.txt" });
}

void tst_FileNameSearchEngine::search_booleanAnd_requiresAllTerms()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/alpha-budget.txt", "alpha-budget.txt", "doc", "txt" },
                                      { rootDir + "/alpha-only.txt", "alpha-only.txt", "doc", "txt" },
                                      { rootDir + "/budget-only.txt", "budget-only.txt", "doc", "txt" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(createBaseOptions(rootDir));

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "alpha", "budget" }, SearchQuery::BooleanOperator::AND));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/alpha-budget.txt" });
}

void tst_FileNameSearchEngine::search_booleanOr_matchesAnyTerm()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/alpha.txt", "alpha.txt", "doc", "txt" },
                                      { rootDir + "/budget.xlsx", "budget.xlsx", "doc", "xlsx" },
                                      { rootDir + "/travel.jpg", "travel.jpg", "pic", "jpg" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(createBaseOptions(rootDir));

    const SearchResultExpected expected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "alpha", "budget" }, SearchQuery::BooleanOperator::OR));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains(rootDir + "/alpha.txt"));
    QVERIFY(paths.contains(rootDir + "/budget.xlsx"));
}

void tst_FileNameSearchEngine::search_wildcard_matchesByPattern()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/Budget-2026.txt", "Budget-2026.txt", "doc", "txt" },
                                      { rootDir + "/Budget-2025.txt", "Budget-2025.txt", "doc", "txt" },
                                      { rootDir + "/Notes-2026.txt", "Notes-2026.txt", "doc", "txt" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    options.setCaseSensitive(false);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(createWildcardQuery("budget-202?.txt"));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains(rootDir + "/Budget-2026.txt"));
    QVERIFY(paths.contains(rootDir + "/Budget-2025.txt"));
}

void tst_FileNameSearchEngine::search_fileTypeFilterOnly_returnsAllMatchingTypes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/report.txt", "report.txt", "doc", "txt" },
                                      { rootDir + "/slides.pptx", "slides.pptx", "doc", "pptx" },
                                      { rootDir + "/holiday.jpg", "holiday.jpg", "pic", "jpg" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    FileNameOptionsAPI api(options);
    api.setFileTypes({ "doc" });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery(QString()));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains(rootDir + "/report.txt"));
    QVERIFY(paths.contains(rootDir + "/slides.pptx"));
}

void tst_FileNameSearchEngine::search_extensionFilterOnly_returnsAllMatchingSuffixes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/one.txt", "one.txt", "doc", "txt" },
                                      { rootDir + "/two.TXT", "two.TXT", "doc", "txt" },
                                      { rootDir + "/three.md", "three.md", "doc", "md" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    FileNameOptionsAPI api(options);
    api.setFileExtensions({ "txt" });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery(QString()));
    QVERIFY(expected.hasValue());

    const QStringList paths = resultPaths(expected);
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains(rootDir + "/one.txt"));
    QVERIFY(paths.contains(rootDir + "/two.TXT"));
}

void tst_FileNameSearchEngine::search_keywordAndTypeFilter_requiresBoth()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/budget.txt", "budget.txt", "doc", "txt" },
                                      { rootDir + "/budget.jpg", "budget.jpg", "pic", "jpg" },
                                      { rootDir + "/notes.txt", "notes.txt", "doc", "txt" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    FileNameOptionsAPI api(options);
    api.setFileTypes({ "doc" });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("budget"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/budget.txt" });
}

void tst_FileNameSearchEngine::search_keywordAndExtensionFilter_requiresBoth()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/budget.txt", "budget.txt", "doc", "txt" },
                                      { rootDir + "/budget.md", "budget.md", "doc", "md" },
                                      { rootDir + "/summary.txt", "summary.txt", "doc", "txt" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    FileNameOptionsAPI api(options);
    api.setFileExtensions({ "txt" });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("budget"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/budget.txt" });
}

void tst_FileNameSearchEngine::search_hiddenFiles_excludedByDefault_andIncludedWhenEnabled()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/visible-plan.txt", "visible-plan.txt", "doc", "txt", "", "", "N" },
                                      { rootDir + "/.hidden-plan.txt", ".hidden-plan.txt", "doc", "txt", "", "", "Y" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));

    SearchOptions defaultOptions = createBaseOptions(rootDir);
    engine->setSearchOptions(defaultOptions);
    const SearchResultExpected defaultExpected = engine->searchSync(SearchQuery::createSimpleQuery("plan"));
    QVERIFY(defaultExpected.hasValue());
    QCOMPARE(resultPaths(defaultExpected), QStringList { rootDir + "/visible-plan.txt" });

    SearchOptions includeHiddenOptions = createBaseOptions(rootDir);
    includeHiddenOptions.setIncludeHidden(true);
    engine->setSearchOptions(includeHiddenOptions);
    const SearchResultExpected includeHiddenExpected = engine->searchSync(SearchQuery::createSimpleQuery("plan"));
    QVERIFY(includeHiddenExpected.hasValue());
    QCOMPARE(resultPaths(includeHiddenExpected).size(), 2);
}

void tst_FileNameSearchEngine::search_excludedPath_filtersSubtreeAtQueryLayer()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString includedDir = rootDir + "/included";
    const QString excludedDir = rootDir + "/excluded";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(includedDir));
    QVERIFY(QDir().mkpath(excludedDir));

    createFileNameIndex(indexDir, {
                                      { includedDir + "/budget.txt", "budget.txt", "doc", "txt" },
                                      { excludedDir + "/budget.txt", "budget.txt", "doc", "txt" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    options.setSearchExcludedPaths({ excludedDir });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("budget"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { includedDir + "/budget.txt" });
}

void tst_FileNameSearchEngine::search_sizeAndTimeFilters_applyOnIndexedFields()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/recent-large.txt", "recent-large.txt", "doc", "txt", "", "", "N", 1712000000, 1700000000, 4096, "4 KB" },
                                      { rootDir + "/recent-small.txt", "recent-small.txt", "doc", "txt", "", "", "N", 1712000000, 1700000000, 128, "128 B" },
                                      { rootDir + "/old-large.txt", "old-large.txt", "doc", "txt", "", "", "N", 1701000000, 1690000000, 4096, "4 KB" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    SizeRangeFilter sizeFilter;
    sizeFilter.setMin(1024);
    options.setSizeRangeFilter(sizeFilter);

    TimeRangeFilter timeFilter;
    timeFilter.setTimeField(TimeField::ModifyTime)
            .setRange(QDateTime::fromSecsSinceEpoch(1711500000),
                      QDateTime::fromSecsSinceEpoch(1712500000));
    options.setTimeRangeFilter(timeFilter);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("recent"));
    QVERIFY(expected.hasValue());

    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/recent-large.txt" });
}

void tst_FileNameSearchEngine::search_pinyinAndAcronym_queriesMatchIndexedFields()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    createFileNameIndex(indexDir, {
                                      { rootDir + "/项目计划.docx", "项目计划.docx", "doc", "docx", "xiangmujihua", "xmjh" },
                                      { rootDir + "/项目总结.docx", "项目总结.docx", "doc", "docx", "xiangmuzongjie", "xmzj" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));

    SearchOptions pinyinOptions = createBaseOptions(rootDir);
    FileNameOptionsAPI pinyinApi(pinyinOptions);
    pinyinApi.setPinyinEnabled(true);
    engine->setSearchOptions(pinyinOptions);

    const SearchResultExpected pinyinExpected = engine->searchSync(SearchQuery::createSimpleQuery("xiangmujihua"));
    QVERIFY(pinyinExpected.hasValue());
    QCOMPARE(resultPaths(pinyinExpected), QStringList { rootDir + "/项目计划.docx" });

    SearchOptions acronymOptions = createBaseOptions(rootDir);
    FileNameOptionsAPI acronymApi(acronymOptions);
    acronymApi.setPinyinAcronymEnabled(true);
    engine->setSearchOptions(acronymOptions);

    const SearchResultExpected acronymExpected = engine->searchSync(SearchQuery::createSimpleQuery("xmjh"));
    QVERIFY(acronymExpected.hasValue());
    QCOMPARE(resultPaths(acronymExpected), QStringList { rootDir + "/项目计划.docx" });
}

void tst_FileNameSearchEngine::search_detailedResults_populatesExtendedAttributes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString indexDir = tempDir.path() + "/filename-index";
    QVERIFY(QDir().mkpath(rootDir));

    const qint64 modifyTime = 1712345678;
    const qint64 birthTime = 1701234567;
    createFileNameIndex(indexDir, {
                                      { rootDir + "/archive.zip", "archive.zip", "archive", "zip", "", "", "N", modifyTime, birthTime, 2048, "2 KB" },
                              });

    stub_ext::StubExt stub;
    stub.set_lamda(DFMSEARCH::Global::fileNameIndexDirectory, [&indexDir]() {
        return indexDir;
    });

    SearchOptions options = createBaseOptions(rootDir);
    options.setDetailedResultsEnabled(true);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("archive"));
    QVERIFY(expected.hasValue());
    QCOMPARE(expected.value().size(), 1);

    SearchResult result = expected.value().first();
    FileNameResultAPI api(result);
    QCOMPARE(result.path(), rootDir + "/archive.zip");
    QCOMPARE(api.filename(), QString("archive.zip"));
    QCOMPARE(api.fileExtension(), QString("zip"));
    QCOMPARE(api.fileType(), QString("archive"));
    QCOMPARE(api.fileSizeBytes(), qint64(2048));
    QCOMPARE(api.size(), QString("2 KB"));
    QCOMPARE(api.modifyTimestamp(), modifyTime);
    QCOMPARE(api.birthTimestamp(), birthTime);
    QCOMPARE(api.isDirectory(), false);
    QCOMPARE(api.isHidden(), false);
}

void tst_FileNameSearchEngine::search_emptyKeywordWithoutFilters_returnsValidationError()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(createBaseOptions(rootDir));

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery(QString()));
    QVERIFY(!expected.hasValue());
    QCOMPARE(expected.error().code().value(), static_cast<int>(FileNameSearchErrorCode::KeywordIsEmpty));
}

void tst_FileNameSearchEngine::search_invalidFileType_returnsValidationError()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));

    SearchOptions options = createBaseOptions(rootDir);
    FileNameOptionsAPI api(options);
    api.setFileTypes({ "invalid-type" });

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("report"));
    QVERIFY(!expected.hasValue());
    QCOMPARE(expected.error().code().value(), static_cast<int>(FileNameSearchErrorCode::InvalidFileTypes));
}

void tst_FileNameSearchEngine::realtime_simpleKeyword_matchesFilesystemEntries()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));
    QVERIFY(createFileWithSize(rootDir + "/alpha-report.txt", 32));
    QVERIFY(createFileWithSize(rootDir + "/meeting-notes.txt", 32));

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(createRealtimeOptions(rootDir));

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("report"));
    QVERIFY(expected.hasValue());
    QCOMPARE(resultPaths(expected), QStringList { rootDir + "/alpha-report.txt" });
}

void tst_FileNameSearchEngine::realtime_booleanAndOr_andWildcard_queriesWork()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));
    QVERIFY(createFileWithSize(rootDir + "/alpha-budget.txt", 16));
    QVERIFY(createFileWithSize(rootDir + "/alpha-only.txt", 16));
    QVERIFY(createFileWithSize(rootDir + "/budget-only.txt", 16));
    QVERIFY(createFileWithSize(rootDir + "/Budget-2026.txt", 16));

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));

    SearchOptions boolOptions = createRealtimeOptions(rootDir);
    engine->setSearchOptions(boolOptions);

    const SearchResultExpected andExpected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "alpha", "budget" }, SearchQuery::BooleanOperator::AND));
    QVERIFY(andExpected.hasValue());
    QCOMPARE(resultPaths(andExpected), QStringList { rootDir + "/alpha-budget.txt" });

    const SearchResultExpected orExpected = engine->searchSync(
            SearchQuery::createBooleanQuery({ "alpha", "budget" }, SearchQuery::BooleanOperator::OR));
    QVERIFY(orExpected.hasValue());
    QCOMPARE(orExpected.value().size(), 4);

    SearchOptions wildcardOptions = createRealtimeOptions(rootDir);
    wildcardOptions.setCaseSensitive(false);
    engine->setSearchOptions(wildcardOptions);

    const SearchResultExpected wildcardExpected = engine->searchSync(createWildcardQuery("budget-202?.txt"));
    QVERIFY(wildcardExpected.hasValue());
    QCOMPARE(resultPaths(wildcardExpected), QStringList { rootDir + "/Budget-2026.txt" });
}

void tst_FileNameSearchEngine::realtime_extensionFilters_areApplied()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));
    QVERIFY(createFileWithSize(rootDir + "/report.txt", 10));
    QVERIFY(createFileWithSize(rootDir + "/slides.pptx", 10));
    QVERIFY(createFileWithSize(rootDir + "/holiday.jpg", 10));

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));

    SearchOptions extOptions = createRealtimeOptions(rootDir);
    FileNameOptionsAPI extApi(extOptions);
    extApi.setFileExtensions({ "txt", "pptx" });
    engine->setSearchOptions(extOptions);

    const SearchResultExpected extExpected = engine->searchSync(SearchQuery::createSimpleQuery(QString()));
    QVERIFY(extExpected.hasValue());
    QCOMPARE(extExpected.value().size(), 2);
}

void tst_FileNameSearchEngine::realtime_hiddenAndExcludedPath_filtersWork()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    const QString includedDir = rootDir + "/included";
    const QString excludedDir = rootDir + "/excluded";
    QVERIFY(QDir().mkpath(includedDir));
    QVERIFY(QDir().mkpath(excludedDir));
    QVERIFY(createFileWithSize(includedDir + "/visible-plan.txt", 8));
    QVERIFY(createFileWithSize(rootDir + "/.hidden-plan.txt", 8));
    QVERIFY(createFileWithSize(excludedDir + "/visible-plan.txt", 8));

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));

    SearchOptions defaultOptions = createRealtimeOptions(rootDir);
    defaultOptions.setSearchExcludedPaths({ excludedDir });
    engine->setSearchOptions(defaultOptions);

    const SearchResultExpected defaultExpected = engine->searchSync(SearchQuery::createSimpleQuery("plan"));
    QVERIFY(defaultExpected.hasValue());
    QCOMPARE(resultPaths(defaultExpected), QStringList { includedDir + "/visible-plan.txt" });

    SearchOptions includeHiddenOptions = createRealtimeOptions(rootDir);
    includeHiddenOptions.setIncludeHidden(true);
    includeHiddenOptions.setSearchExcludedPaths({ excludedDir });
    engine->setSearchOptions(includeHiddenOptions);

    const SearchResultExpected includeHiddenExpected = engine->searchSync(SearchQuery::createSimpleQuery("plan"));
    QVERIFY(includeHiddenExpected.hasValue());
    const QStringList paths = resultPaths(includeHiddenExpected);
    QCOMPARE(paths.size(), 2);
    QVERIFY(paths.contains(includedDir + "/visible-plan.txt"));
    QVERIFY(paths.contains(rootDir + "/.hidden-plan.txt"));
}

void tst_FileNameSearchEngine::realtime_sizeAndTimeFilters_applyWithoutIndex()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));

    const QString recentLarge = rootDir + "/recent-large.txt";
    const QString recentSmall = rootDir + "/recent-small.txt";
    const QString oldLarge = rootDir + "/old-large.txt";
    QVERIFY(createFileWithSize(recentLarge, 4096));
    QVERIFY(createFileWithSize(recentSmall, 128));
    QVERIFY(createFileWithSize(oldLarge, 4096));

    const QDateTime recentTime = QDateTime::fromSecsSinceEpoch(1712000000);
    const QDateTime oldTime = QDateTime::fromSecsSinceEpoch(1701000000);
    QFile recentLargeFile(recentLarge);
    QFile recentSmallFile(recentSmall);
    QFile oldLargeFile(oldLarge);
    QVERIFY(recentLargeFile.open(QIODevice::ReadWrite));
    QVERIFY(recentSmallFile.open(QIODevice::ReadWrite));
    QVERIFY(oldLargeFile.open(QIODevice::ReadWrite));
    QVERIFY(recentLargeFile.setFileTime(recentTime, QFileDevice::FileModificationTime));
    QVERIFY(recentSmallFile.setFileTime(recentTime, QFileDevice::FileModificationTime));
    QVERIFY(oldLargeFile.setFileTime(oldTime, QFileDevice::FileModificationTime));
    recentLargeFile.close();
    recentSmallFile.close();
    oldLargeFile.close();

    SearchOptions options = createRealtimeOptions(rootDir);
    SizeRangeFilter sizeFilter;
    sizeFilter.setMin(1024);
    options.setSizeRangeFilter(sizeFilter);

    TimeRangeFilter timeFilter;
    timeFilter.setTimeField(TimeField::ModifyTime)
            .setRange(QDateTime::fromSecsSinceEpoch(1711500000),
                      QDateTime::fromSecsSinceEpoch(1712500000));
    options.setTimeRangeFilter(timeFilter);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("recent"));
    QVERIFY(expected.hasValue());
    QCOMPARE(resultPaths(expected), QStringList { recentLarge });
}

void tst_FileNameSearchEngine::realtime_detailedResults_populateAttributes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));
    const QString archivePath = rootDir + "/archive.zip";
    QVERIFY(createFileWithSize(archivePath, 2048));

    SearchOptions options = createRealtimeOptions(rootDir);
    options.setDetailedResultsEnabled(true);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("archive"));
    QVERIFY(expected.hasValue());
    QCOMPARE(expected.value().size(), 1);

    SearchResult result = expected.value().first();
    FileNameResultAPI api(result);
    QCOMPARE(result.path(), archivePath);
    QCOMPARE(api.filename(), QString("archive.zip"));
    QCOMPARE(api.fileExtension(), QString("zip"));
    QCOMPARE(api.fileType(), QString("zip"));
    QCOMPARE(api.fileSizeBytes(), qint64(2048));
    QCOMPARE(api.isDirectory(), false);
    QCOMPARE(api.isHidden(), false);
    QVERIFY(api.modifyTimestamp() > 0);
}

void tst_FileNameSearchEngine::realtime_pinyinOption_doesNotProducePinyinMatches()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path() + "/docs";
    QVERIFY(QDir().mkpath(rootDir));
    QVERIFY(createFileWithSize(rootDir + "/项目计划.docx", 32));

    SearchOptions options = createRealtimeOptions(rootDir);
    FileNameOptionsAPI api(options);
    api.setPinyinEnabled(true);

    std::unique_ptr<SearchEngine> engine(SearchEngine::create(SearchType::FileName));
    engine->setSearchOptions(options);

    const SearchResultExpected expected = engine->searchSync(SearchQuery::createSimpleQuery("xiangmujihua"));
    QVERIFY(expected.hasValue());
    QCOMPARE(expected.value().size(), 0);
}

QObject *create_tst_FileNameSearchEngine()
{
    return new tst_FileNameSearchEngine();
}

#include "tst_filename_search_engine.moc"
