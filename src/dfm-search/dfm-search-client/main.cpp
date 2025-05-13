// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QStringList>
#include <QFileInfo>
#include <QDir>

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
// dfm6-search-client --file-types=doc,pic "" /home/user

// # 文件后缀过滤
// dfm6-search-client --file-extensions=txt,pdf "" /home/user

// # 文件类型和后缀组合过滤
// dfm6-search-client --file-types=doc --file-extensions=docx,odt "report" /home/user

// # 布尔查询
// dfm6-search-client --query=boolean "meeting,notes,2023" /home/user/Documents

// # Combined
// dfm6-search-client --file-types="dir,doc" --query=boolean "dde,file" /
// dfm6-search-client --pinyin --query=boolean "wendang,xinjian" /
// dfm6-search-client --pinyin --file-types="doc,pic" --query=boolean "wen,dang" /
// dfm6-search-client --file-extensions="txt,pdf" --query=boolean "report,data" /

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
    std::cout << "  --file-extensions=<exts>    Filter by file extensions, comma separated" << std::endl;
    std::cout << "  --max-results=<number>      Maximum number of results" << std::endl;
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

void testGlobal()
{
    std::cout << "================= test global start =================" << std::endl;

    // Test supported content search extensions
    QStringList testExtensions = { "txt", "pdf", "docx", "unknown" };
    for (const auto &ext : testExtensions) {
        std::cout << "Check if '" << ext.toStdString() << "' is supported: "
                  << (Global::isSupportedContentSearchExtension(ext) ? "Yes" : "No") << std::endl;
    }

    // Test default content search extensions
    QStringList defaultExtensions = Global::defaultContentSearchExtensions();
    std::cout << "Default supported content search extensions: "
              << defaultExtensions.join(", ").toStdString() << std::endl;

    // Test content index availability
    std::cout << "Is content index available: "
              << (Global::isContentIndexAvailable() ? "Yes" : "No") << std::endl;

    // Test content index directory
    std::cout << "Content index directory: "
              << Global::contentIndexDirectory().toStdString() << std::endl;

    // Test path in content index directory
    QString testPath = QDir::homePath() + "/test.txt";
    std::cout << "Is path in content index directory: "
              << (Global::isPathInContentIndexDirectory(testPath) ? "Yes" : "No") << std::endl;

    // Test filename index directory availability
    std::cout << "Is filename index directory available: "
              << (Global::isFileNameIndexDirectoryAvailable() ? "Yes" : "No") << std::endl;

    // Test filename index directory
    std::cout << "Filename index directory: "
              << Global::fileNameIndexDirectory().toStdString() << std::endl;

    std::cout << "Default indexed dirs: " << std::endl;
    const auto &dirs = Global::defaultIndexedDirectory();
    for (const auto &dir : dirs) {
        std::cout << dir.toStdString() << std::endl;
    }

    std::cout << "================= test global end =================" << std::endl;
}

void doTestPinyin(const std::string &caseName, const QString &input, bool expected)
{
    bool actual = Global::isPinyinSequence(input);
    std::cout << "测试用例 [" << caseName << "]\t"
              << "输入值: " << input.toStdString() << "\t"
              << "预期结果: " << (expected ? "有效" : "无效") << "\t"
              << "实际结果: " << (actual ? "有效" : "无效") << "\t"
              << "状态: " << (actual == expected ? "通过" : "失败") << "\n"
              << "---------------------------------\n";
}

void testPinyin()
{
    // 有效拼音测试集
    std::vector<std::pair<QString, bool>> validCases = {
        // 单韵母
        { "a", true },   // 单韵母
        { "o", true },   // 单韵母
        { "e", true },   // 单韵母
        { "O", true },   // 大写单韵母

        // 声母+韵母
        { "ba", true },   // 普通音节
        { "po", true },   // 普通音节
        { "mi", true },   // 普通音节

        // 特殊音节
        { "zhi", true },   // 整体认读音节
        { "chi", true },   // 整体认读音节
        { "shi", true },   // 整体认读音节

        // 复韵母
        { "ai", true },   // 复韵母
        { "er", true },   // 儿化音
        { "ang", true },   // 复韵母

        // ü相关
        { "lv", true },   // ü的v替代写法
        { "nü", true },   // ü的unicode写法
        { "lüe", true },   // ü开头的复韵母

        // 多音节
        { "nihao", true },   // 常见词
        { "pinyin", true },   // 常见词
        { "zhongwen", true },   // 常见词
        { "shuang", true },   // 双声母+复韵母
        { "xian", true },   // 特殊声母规则
        { "quan", true },   // 特殊声母规则
        { "jiang", true },   // 特殊声母规则

        // 大小写混合
        { "ZhongGuo", true },   // 大小写混合
        { "XIONG", true },   // 全大写
        { "PinYin", true },   // 大小写混合

        { "make", true },
        { "xinjian", true },
        { "zhangsheng", true },
        { "wendan", true },
        { "wendang", true },
        { "xiaa", true },
        { "chaojichangdeyijuhua", true },
        { "chengong", true },
        { "shibai", true },
        { "case", true },
        { "sougou", true },
        { "sousuo", true },
        { "jieshi", true },
        { "zongjie", true },
        { "jiu", true },
        { "chengdu", true },
        { "beijing", true },
        { "xian", true },
        { "chongqing", true },
        { "chongqin", true },
        { "chenqin", true },
        { "shanghai", true },
    };

    // 无效拼音测试集
    std::vector<std::pair<QString, bool>> invalidCases = {
        // 基本无效情况
        { "", false },   // 空字符串
        { "vvv", false },   // 重复字母
        { "kkkk", false },   // 重复声母
        { "i", false },   // i不能单独成音节
        { "u", false },   // u不能单独成音节

        // 非法拼音组合
        { "xqiong", false },   // 非法声母组合
        { "jin'an", false },   // 包含特殊字符
        { "ni hao", false },   // 包含空格

        // 英文单词
        { "hello", false },   // 英文单词
        { "world", false },   // 英文单词
        { "cmake", false },   // 英文单词

        // 数字和特殊字符
        { "ni3hao", false },   // 包含数字
        { "zh@ng", false },   // 包含特殊字符
        { "pin-yin", false },   // 包含连字符

        // 不完整或错误的拼音
        { "zh", false },   // 只有声母
        { "zho", false },   // 非法组合
        { "jx", false },   // 非法组合

        // 特殊规则测试
        { "yi", true },   // 整体认读音节
        { "wu", true },   // 整体认读音节
        { "yu", true },   // 整体认读音节
        { "yue", true },   // 特殊组合
        { "yuan", true },   // 特殊组合

        // 边界情况
        { "v", false },   // 单个v
        { "ü", false },   // 单个ü
        { "ng", false },   // 非法音节
        { "gn", false },   // 非法音节

        { "123", false },
        { "ni*hao", false },
        { "z", false },
        { "zh", false },
        { "p", false },
        { "m", false },
        { "b", false },
        { "jiv", false },

    };

    std::cout << "====== 开始拼音有效性测试 ======\n";
    for (const auto &[input, expected] : validCases) {
        doTestPinyin("有效拼音验证", input, expected);
    }

    std::cout << "====== 开始拼音无效性测试 ======\n";
    for (const auto &[input, expected] : invalidCases) {
        doTestPinyin("无效拼音检测", input, expected);
    }
}

void testAnythingStatus()
{
    qDebug() << "=== Starting Anything Status Test ===";

    // 测试状态获取
    auto status = Global::fileNameIndexStatus();

    // 结果验证
    if (!status.has_value()) {
        qWarning() << "Test Failed: Could not retrieve status (file missing/permission error?)";
        return;
    }

    // 定义有效状态列表（小写）
    static const QSet<QString> validStatuses {
        "loading",
        "scanning",
        "monitoring",
        "closed"
    };

    // 状态有效性检查
    const QString currentStatus = status.value();
    if (!validStatuses.contains(currentStatus)) {
        qWarning() << "Test Failed: Invalid anything status value:" << currentStatus
                   << "\nExpected one of:" << validStatuses.values();
        return;
    }

    // 成功输出
    qInfo() << "Test Passed. Current anything status:" << currentStatus;

    // 附加信息：打印时间戳（如果测试需要）
    /*
    QString statusFile = QDir(Global::fileNameIndexDirectory()).filePath("status.json");
    QFile file(statusFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
        qDebug() << "Last update:" << json["time"].toString();
    }
    */
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef QT_DEBUG
    testGlobal();
    testPinyin();
    testAnythingStatus();
#endif

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
    QCommandLineOption fileExtensionsOption(QStringList() << "file-extensions", "Filter by file extensions, comma separated", "extensions");
    QCommandLineOption maxResultsOption(QStringList() << "max-results", "Maximum number of results", "number", "100");
    QCommandLineOption maxPreviewOption(QStringList() << "max-preview", "Max content preview length", "length", "200");

    parser.addOption(typeOption);
    parser.addOption(methodOption);
    parser.addOption(queryOption);
    parser.addOption(caseSensitiveOption);
    parser.addOption(includeHiddenOption);
    parser.addOption(pinyinOption);
    parser.addOption(fileTypesOption);
    parser.addOption(fileExtensionsOption);
    parser.addOption(maxResultsOption);
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
    if (searchMethod == SearchMethod::Realtime)
        options.setResultFoundEnabled(true);

    // Set max results if specified
    if (parser.isSet(maxResultsOption)) {
        bool ok;
        int maxResults = parser.value(maxResultsOption).toInt(&ok);
        if (ok && maxResults > 0) {
            options.setMaxResults(maxResults);
        }
    }

    // Set type-specific options
    if (searchType == SearchType::FileName) {
        FileNameOptionsAPI fileNameOptions(options);
        fileNameOptions.setPinyinEnabled(parser.isSet(pinyinOption));

        if (parser.isSet(fileTypesOption)) {
            QStringList types = parser.value(fileTypesOption).split(',', Qt::SkipEmptyParts);
            fileNameOptions.setFileTypes(types);
        }

        if (parser.isSet(fileExtensionsOption)) {
            QStringList extensions = parser.value(fileExtensionsOption).split(',', Qt::SkipEmptyParts);
            fileNameOptions.setFileExtensions(extensions);
        }
    } else if (searchType == SearchType::Content) {
        ContentOptionsAPI contentOptions(options);

        // if (parser.isSet(fileTypesOption)) {
        //     QStringList extensions = parser.value(fileTypesOption).split(',', Qt::SkipEmptyParts);
        //     contentOptions.setFileTypeFilters(extensions);
        // }
        contentOptions.setMaxPreviewLength(200);
        contentOptions.setFullTextRetrievalEnabled(true);
        contentOptions.setSearchResultHighlightEnabled(true);
        contentOptions.setFilenameContentMixedAndSearchEnabled(true);
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

    QObject::connect(engine, &SearchEngine::resultsFound, [searchType](const SearchResultList &results) {
        for (const auto &result : results)
            printSearchResult(result, searchType);
    });

    QObject::connect(engine, &SearchEngine::searchFinished, [options, searchType](const QList<SearchResult> &results) {
        std::cout << "Search finished. Total results: " << results.size() << std::endl;
        if (!options.resultFoundEnabled()) {
            std::for_each(results.begin(), results.end(), [searchType](const SearchResult &result) {
                printSearchResult(result, searchType);
            });
        }
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
    });

    // Start search
    std::cout << "Searching for: " << keyword.toStdString() << std::endl;
    std::cout << "In path: " << searchPath.toStdString() << std::endl;
    std::cout << "Search type: " << (searchType == SearchType::FileName ? "Filename" : "Content") << std::endl;
    std::cout << "Search method: " << (searchMethod == SearchMethod::Indexed ? "Indexed" : "Realtime") << std::endl;

    // Print file extensions if set
    if (searchType == SearchType::FileName && parser.isSet(fileExtensionsOption)) {
        std::cout << "File extensions filter: " << parser.value(fileExtensionsOption).toStdString() << std::endl;
    }

    engine->search(query);

    return a.exec();
}
