// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QStringList>
#include <QFileInfo>
#include <iostream>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchengine.h>
#include <dfm-search/searchfactory.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/contentsearchapi.h>

using namespace dfmsearch;

// # 基本文件名搜索
// dfm6-search-client "document" /home/user

// # 内容搜索
// dfm6-search-client --type=content "hello world" /home/user/Documents

// # 使用realtime搜索
// dfm6-search-client --method=realtime "report" /home/user

// # 区分大小写的搜索
// dfm6-search-client --case-sensitive "README" /home/user

// # 文件类型过滤
// dfm6-search-client --file-types=txt,doc,docx /home/user

// # 布尔查询
// dfm6-search-client --query=boolean "meeting,notes,2023" /home/user/Documents

void printUsage()
{
    std::cout << "Usage: dfm6-search-client [options] <keyword> <search_path>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --type=<filename|content>   Search type (default: filename)" << std::endl;
    std::cout << "  --method=<indexed|realtime> Search method (default: indexed)" << std::endl;
    std::cout << "  --query=<simple|boolean>    Query type (default: simple)" << std::endl;
    std::cout << "  --case-sensitive            Enable case sensitivity" << std::endl;
    std::cout << "  --include-hidden            Include hidden files" << std::endl;
    std::cout << "  --pinyin                    Enable pinyin search (for filename search)" << std::endl;
    std::cout << "  --file-types=<types>        Filter by file types, comma separated" << std::endl;
    std::cout << "  --max-results=<number>      Maximum number of results" << std::endl;
    std::cout << "  --exclude-path=<path>       Path to exclude (can be used multiple times)" << std::endl;
    std::cout << "  --max-preview=<length>      Max content preview length (for content search)" << std::endl;
    std::cout << "  --help                      Display this help" << std::endl;
}

void printSearchResult(const SearchResult &result, SearchType searchType)
{
    std::cout << "Found: " << result.path().toStdString() << std::endl;

    if (searchType == SearchType::FileName) {
        FileNameResultAPI resultAPI(const_cast<SearchResult &>(result));

        if (resultAPI.isDirectory()) {
            std::cout << "  Type: Directory" << std::endl;
        } else {
            std::cout << "  Type: " << resultAPI.fileType().toStdString() << std::endl;
            std::cout << "  Size: " << resultAPI.size().toStdString() << " bytes" << std::endl;
        }

        std::cout << "  Modified: " << resultAPI.modifiedTime().toStdString() << std::endl;
    } else if (searchType == SearchType::Content) {
        ContentResultAPI contentResult(const_cast<SearchResult &>(result));
        std::cout << "  Content match: " << contentResult.highlightedContent().toStdString() << std::endl;
    }

    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("DFM Search Client");
    parser.addHelpOption();

    // Add command line options
    QCommandLineOption typeOption(QStringList() << "type", "Search type (filename or content)", "type", "filename");
    QCommandLineOption methodOption(QStringList() << "method", "Search method (indexed or realtime)", "method", "indexed");
    QCommandLineOption queryOption(QStringList() << "query", "Query type (simple or boolean)", "query", "simple");
    QCommandLineOption caseSensitiveOption(QStringList() << "case-sensitive", "Enable case sensitivity");
    QCommandLineOption includeHiddenOption(QStringList() << "include-hidden", "Include hidden files");
    QCommandLineOption pinyinOption(QStringList() << "pinyin", "Enable pinyin search (for filename search)");
    QCommandLineOption fileTypesOption(QStringList() << "file-types", "Filter by file types, comma separated", "types");
    QCommandLineOption maxResultsOption(QStringList() << "max-results", "Maximum number of results", "number", "100");
    QCommandLineOption excludePathOption(QStringList() << "exclude-path", "Path to exclude", "path");
    QCommandLineOption maxPreviewOption(QStringList() << "max-preview", "Max content preview length", "length", "200");

    parser.addOption(typeOption);
    parser.addOption(methodOption);
    parser.addOption(queryOption);
    parser.addOption(caseSensitiveOption);
    parser.addOption(includeHiddenOption);
    parser.addOption(pinyinOption);
    parser.addOption(fileTypesOption);
    parser.addOption(maxResultsOption);
    parser.addOption(excludePathOption);
    parser.addOption(maxPreviewOption);

    // Setup positional arguments
    parser.addPositionalArgument("keyword", "Search keyword");
    parser.addPositionalArgument("search_path", "Path to search in");

    // Process arguments
    parser.process(a);

    QStringList positionalArgs = parser.positionalArguments();
    if (positionalArgs.size() < 2) {
        printUsage();
        return 1;
    }

    QString keyword = positionalArgs.at(0);
    QString searchPath = positionalArgs.at(1);

    // Validate search path
    QFileInfo pathInfo(searchPath);
    if (!pathInfo.exists() || !pathInfo.isDir()) {
        std::cerr << "Error: Search path does not exist or is not a directory" << std::endl;
        return 1;
    }

    // Get search type
    SearchType searchType = SearchType::FileName;
    QString typeStr = parser.value(typeOption);
    if (typeStr == "content") {
        searchType = SearchType::Content;
    } else if (typeStr != "filename") {
        std::cerr << "Error: Invalid search type. Use 'filename' or 'content'" << std::endl;
        return 1;
    }

    // Get search method
    SearchMethod searchMethod = SearchMethod::Indexed;
    QString methodStr = parser.value(methodOption);
    if (methodStr == "realtime") {
        searchMethod = SearchMethod::Realtime;
    } else if (methodStr != "indexed") {
        std::cerr << "Error: Invalid search method. Use 'indexed' or 'realtime'" << std::endl;
        return 1;
    }

    // Get query type
    SearchQuery::Type queryType = SearchQuery::Type::Simple;
    QString queryStr = parser.value(queryOption);
    if (queryStr == "boolean") {
        queryType = SearchQuery::Type::Boolean;
    } else if (queryStr != "simple") {
        std::cerr << "Error: Invalid query type. Use 'simple' or 'boolean'" << std::endl;
        return 1;
    }

    // Create search engine
    SearchEngine *engine = SearchFactory::createEngine(searchType, &a);
    if (!engine) {
        std::cerr << "Error: Failed to create search engine" << std::endl;
        return 1;
    }

    // Setup search options
    SearchOptions options;
    options.setSearchMethod(searchMethod);
    options.setCaseSensitive(parser.isSet(caseSensitiveOption));
    options.setIncludeHidden(parser.isSet(includeHiddenOption));
    options.setSearchPath(searchPath);

    // Set max results if specified
    if (parser.isSet(maxResultsOption)) {
        bool ok;
        int maxResults = parser.value(maxResultsOption).toInt(&ok);
        if (ok && maxResults > 0) {
            options.setMaxResults(maxResults);
        }
    }

    // Add exclude paths if specified
    if (parser.isSet(excludePathOption)) {
        QStringList excludePaths = parser.values(excludePathOption);
        options.setExcludePaths(excludePaths);
    }

    // Set type-specific options
    if (searchType == SearchType::FileName) {
        FileNameOptionsAPI fileNameOptions(options);
        fileNameOptions.setPinyinEnabled(parser.isSet(pinyinOption));

        if (parser.isSet(fileTypesOption)) {
            QStringList types = parser.value(fileTypesOption).split(',', Qt::SkipEmptyParts);
            fileNameOptions.setFileTypes(types);
        }
    } else if (searchType == SearchType::Content) {
        ContentOptionsAPI contentOptions(options);

        if (parser.isSet(fileTypesOption)) {
            QStringList extensions = parser.value(fileTypesOption).split(',', Qt::SkipEmptyParts);
            contentOptions.setFileTypeFilters(extensions);
        }

        if (parser.isSet(maxPreviewOption)) {
            bool ok;
            int previewLength = parser.value(maxPreviewOption).toInt(&ok);
            if (ok && previewLength > 0) {
                contentOptions.setMaxPreviewLength(previewLength);
            }
        }
    }

    engine->setSearchOptions(options);

    // Create and configure search query
    SearchQuery query;
    if (queryType == SearchQuery::Type::Simple) {
        query = SearchFactory::createQuery(keyword, SearchQuery::Type::Simple);
    } else {
        // For boolean query, split keywords by comma and create a boolean query
        QStringList keywords = keyword.split(',', Qt::SkipEmptyParts);
        query = SearchFactory::createQuery(keywords, SearchQuery::Type::Boolean);
        query.setBooleanOperator(SearchQuery::BooleanOperator::AND);
    }

    // Connect signals
    QObject::connect(engine, &SearchEngine::searchStarted, [] {
        std::cout << "Search started..." << std::endl;
    });

    QObject::connect(engine, &SearchEngine::resultFound, [searchType](const SearchResult &result) {
        printSearchResult(result, searchType);
    });

    QObject::connect(engine, &SearchEngine::progressChanged, [](int current, int total) {
        std::cout << "Progress: " << current << "/" << total << std::endl;
    });

    QObject::connect(engine, &SearchEngine::searchFinished, [](const QList<SearchResult> &results) {
        std::cout << "Search finished. Total results: " << results.size() << std::endl;
        QCoreApplication::quit();
    });

    QObject::connect(engine, &SearchEngine::searchCancelled, [] {
        std::cout << "Search cancelled" << std::endl;
        QCoreApplication::quit();
    });

    QObject::connect(engine, &SearchEngine::errorOccurred, [](const DFMSEARCH::SearchError &error) {
        std::cerr << "[Error]: " << error.code()
                  << "[Name]: " << error.name().toStdString()
                  << "[Message]:" << error.message().toStdString() << std::endl;
        QCoreApplication::quit();
    });

    // Start search
    std::cout << "Searching for: " << keyword.toStdString() << std::endl;
    std::cout << "In path: " << searchPath.toStdString() << std::endl;
    std::cout << "Search type: " << (searchType == SearchType::FileName ? "Filename" : "Content") << std::endl;
    std::cout << "Search method: " << (searchMethod == SearchMethod::Indexed ? "Indexed" : "Realtime") << std::endl;

    engine->search(query);

    return a.exec();
}
