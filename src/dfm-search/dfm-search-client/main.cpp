// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
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
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QTimer>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchengine.h>
#include <dfm-search/searchfactory.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/contentsearchapi.h>
#include "../dfm-search-lib/utils/filenameblacklistmatcher.h"

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
// dfm6-search-client --pinyin-acronym "nh" /home/user  # 搜索"你好"的拼音首字母
// dfm6-search-client --pinyin-acronym --query=boolean "wd,xj" /  # 搜索"文档,新建"的拼音首字母
// dfm6-search-client --pinyin --pinyin-acronym "wendang" /  # 智能模式：有效拼音用拼音搜索
// dfm6-search-client --pinyin --pinyin-acronym "wd" /  # 智能模式：有效首字母用首字母搜索
// dfm6-search-client --pinyin --pinyin-acronym "nh123" /  # 智能模式：首字母+数字用首字母搜索
// dfm6-search-client --pinyin --pinyin-acronym "abc@#" /  # 智能模式：无效输入fallback到普通搜索
// dfm6-search-client --file-extensions="txt,pdf" --query=boolean "report,data" /

void printUsage()
{
    std::cout << "Usage: dfm6-search-client [options] <keyword> <search_path>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --type=<filename|content>   Search type (default: filename)" << std::endl;
    std::cout << "  --method=<indexed|realtime> Search method (default: indexed)" << std::endl;
    std::cout << "  --query=<simple|boolean|wildcard> Query type (default: simple)" << std::endl;
    std::cout << "  --wildcard                  Enable wildcard search with * and ? patterns" << std::endl;
    std::cout << "  --case-sensitive            Enable case sensitivity" << std::endl;
    std::cout << "  --include-hidden            Include hidden files" << std::endl;
    std::cout << "  --pinyin                    Enable pinyin search (for filename search)" << std::endl;
    std::cout << "  --pinyin-acronym            Enable pinyin acronym search (for filename search)" << std::endl;
    std::cout << "  --file-types=<types>        Filter by file types, comma separated" << std::endl;
    std::cout << "  --file-extensions=<exts>    Filter by file extensions, comma separated" << std::endl;
    std::cout << "  --max-results=<number>      Maximum number of results" << std::endl;
    std::cout << "  --max-preview=<length>      Max content preview length (for content search)" << std::endl;
    std::cout << "  --json, -j                  Output results in JSON format" << std::endl;
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

//--------------------------------------------------------------------
// JSON Output Helpers
//--------------------------------------------------------------------

QJsonValue resultToJson(const SearchResult &result, SearchType searchType)
{
    if (searchType == SearchType::FileName) {
        // 文件名搜索：直接返回路径字符串
        return result.path();
    } else if (searchType == SearchType::Content) {
        // 内容搜索：返回包含路径和内容匹配的对象
        QJsonObject obj;
        obj["path"] = result.path();
        ContentResultAPI contentResult(const_cast<SearchResult &>(result));
        obj["contentMatch"] = contentResult.highlightedContent();
        return obj;
    }
    return result.path();
}

void printJsonLine(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    std::cout << doc.toJson(QJsonDocument::Compact).constData() << std::endl;
}

// 流式模式：输出搜索开始
void printJsonSearchStart(const QString &keyword, const QString &searchPath,
                          SearchType searchType, SearchMethod searchMethod,
                          const SearchOptions &options)
{
    QJsonObject startObj;
    startObj["type"] = "search_started";

    QJsonObject searchInfo;
    searchInfo["keyword"] = keyword;
    searchInfo["searchPath"] = searchPath;
    searchInfo["searchType"] = (searchType == SearchType::FileName ? "filename" : "content");
    searchInfo["searchMethod"] = (searchMethod == SearchMethod::Indexed ? "indexed" : "realtime");
    searchInfo["caseSensitive"] = options.caseSensitive();
    searchInfo["includeHidden"] = options.includeHidden();

    startObj["search"] = searchInfo;
    startObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    printJsonLine(startObj);
}

// 流式模式：输出单个结果
void printJsonResult(const SearchResult &result, SearchType searchType)
{
    QJsonObject resultObj;
    resultObj["type"] = "result";
    resultObj["data"] = resultToJson(result, searchType);
    printJsonLine(resultObj);
}

// 流式模式：输出搜索结束
void printJsonSearchEnd(int totalResults, qint64 elapsedMs)
{
    QJsonObject endObj;
    endObj["type"] = "search_finished";

    QJsonObject status;
    status["state"] = "success";
    status["totalResults"] = totalResults;

    endObj["status"] = status;

    QJsonObject timestamps;
    timestamps["finished"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    timestamps["duration"] = elapsedMs;

    endObj["timestamps"] = timestamps;
    printJsonLine(endObj);
}

// 完整 JSON 输出（非流式）
struct JsonOutputContext
{
    QString keyword;
    QString searchPath;
    SearchType searchType;
    SearchMethod searchMethod;
    SearchOptions options;
    QDateTime startTime;
    QJsonArray results;
};

void printJsonComplete(const JsonOutputContext &ctx)
{
    QJsonObject root;

    // 搜索信息
    QJsonObject searchInfo;
    searchInfo["keyword"] = ctx.keyword;
    searchInfo["searchPath"] = ctx.searchPath;
    searchInfo["searchType"] = (ctx.searchType == SearchType::FileName ? "filename" : "content");
    searchInfo["searchMethod"] = (ctx.searchMethod == SearchMethod::Indexed ? "indexed" : "realtime");
    searchInfo["caseSensitive"] = ctx.options.caseSensitive();
    searchInfo["includeHidden"] = ctx.options.includeHidden();
    root["search"] = searchInfo;

    // 时间戳
    QDateTime endTime = QDateTime::currentDateTime();
    qint64 duration = ctx.startTime.msecsTo(endTime);

    QJsonObject timestamps;
    timestamps["started"] = ctx.startTime.toString(Qt::ISODate);
    timestamps["finished"] = endTime.toString(Qt::ISODate);
    timestamps["duration"] = duration;
    root["timestamps"] = timestamps;

    // 状态
    QJsonObject status;
    status["state"] = "success";
    status["totalResults"] = ctx.results.size();
    root["status"] = status;

    // 结果数组
    root["results"] = ctx.results;

    // 输出完整 JSON
    QJsonDocument doc(root);
    std::cout << doc.toJson(QJsonDocument::Indented).constData() << std::endl;
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

    std::cout << "Default blacklist paths: " << std::endl;
    const auto &blacklistPaths = Global::defaultBlacklistPaths();
    for (const auto &path : blacklistPaths) {
        std::cout << "  " << path.toStdString() << std::endl;
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

void doTestPinyinAcronym(const std::string &caseName, const QString &input, bool expected)
{
    bool actual = Global::isPinyinAcronymSequence(input);
    std::cout << "拼音首字母测试 [" << caseName << "]\t"
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

        // 英文单词
        { "hello", false },   // 英文单词
        { "world", false },   // 英文单词
        { "cmake", false },   // 英文单词

        // 数字和特殊字符
        { "zh@ng", false },   // 包含特殊字符

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

void testPinyinAcronym()
{
    std::cout << "====== 开始拼音首字母有效性测试 ======\n";

    // 有效拼音首字母测试集
    std::vector<std::pair<QString, bool>> validCases = {
        // 基本有效情况
        { "n", true },   // 单字符
        { "nh", true },   // 双字符（你好）
        { "wd", true },   // 双字符（文档）
        { "xj", true },   // 双字符（新建）
        { "zhzw", true },   // 四字符（中文）
        { "dfm", true },   // 三字符
        { "ABC", true },   // 大写字母
        { "AbC", true },   // 大小写混合
        { "hello", true },   // 英文单词形式
        { "a", true },   // 单个字母
        { "z", true },   // 单个字母
        // 包含数字和符号的有效情况
        { "nh123", true },   // 拼音首字母+数字（你好123）
        { "wd_v1", true },   // 拼音首字母+下划线+版本号
        { "test-file", true },   // 英文+连字符
        { "config.bak", true },   // 英文+点号
        { "a1b2c3", true },   // 字母数字混合
        { "file_2023", true },   // 文件名模式
    };

    // 无效拼音首字母测试集
    std::vector<std::pair<QString, bool>> invalidCases = {
        // 基本无效情况
        { "", false },   // 空字符串
        { "你好", false },   // 中文
        { "n好", false },   // 中文
        { "123", false },   // 纯数字
        { "._-", false },   // 纯符号
    };

    for (const auto &[input, expected] : validCases) {
        doTestPinyinAcronym("有效首字母验证", input, expected);
    }

    for (const auto &[input, expected] : invalidCases) {
        doTestPinyinAcronym("无效首字母检测", input, expected);
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

void doFileNameBlacklistMatchTest(const std::string &caseName,
                                  const QString &inputPath,
                                  const QStringList &blacklistEntries,
                                  bool expected)
{
    const bool actual = Global::BlacklistMatcher::isPathBlacklisted(inputPath, blacklistEntries);
    std::cout << "文件名黑名单测试 [" << caseName << "]\t"
              << "输入路径: " << inputPath.toStdString() << "\t"
              << "黑名单: " << blacklistEntries.join(", ").toStdString() << "\t"
              << "预期结果: " << (expected ? "命中" : "不命中") << "\t"
              << "实际结果: " << (actual ? "命中" : "不命中") << "\t"
              << "状态: " << (actual == expected ? "通过" : "失败") << "\n"
              << "---------------------------------\n";
}

void testFileNameBlacklistMatcher()
{
    std::cout << "====== 开始文件名黑名单规则测试 ======\n";

    doFileNameBlacklistMatchTest("绝对路径-自身命中",
                                 "/home/test/workspace",
                                 { "/home/test/workspace" },
                                 true);

    doFileNameBlacklistMatchTest("绝对路径-子路径命中",
                                 "/home/test/workspace/a.txt",
                                 { "/home/test/workspace" },
                                 true);

    doFileNameBlacklistMatchTest("绝对路径-边界不误匹配",
                                 "/home/test/workspace2",
                                 { "/home/test/workspace" },
                                 false);

    doFileNameBlacklistMatchTest("目录名-直接命中",
                                 "/home/test/workspace",
                                 { "workspace" },
                                 true);

    doFileNameBlacklistMatchTest("目录名-深层命中",
                                 "/home/test/aa/bb/workspace",
                                 { "workspace" },
                                 true);

    doFileNameBlacklistMatchTest("目录名-不命中相似名称",
                                 "/home/test/aa/bb/myworkspace",
                                 { "workspace" },
                                 false);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef QT_DEBUG
    testGlobal();
    testPinyin();
    testPinyinAcronym();
    testAnythingStatus();
    testFileNameBlacklistMatcher();
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
    QCommandLineOption pinyinAcronymOption(QStringList() << "pinyin-acronym", "Enable pinyin acronym search (for filename search)");
    QCommandLineOption fileTypesOption(QStringList() << "file-types", "Filter by file types, comma separated", "types");
    QCommandLineOption fileExtensionsOption(QStringList() << "file-extensions", "Filter by file extensions, comma separated", "extensions");
    QCommandLineOption maxResultsOption(QStringList() << "max-results", "Maximum number of results", "number", "100");
    QCommandLineOption maxPreviewOption(QStringList() << "max-preview", "Max content preview length", "length", "200");
    QCommandLineOption wildcardOption(QStringList() << "wildcard", "Enable wildcard search with * and ? patterns");
    QCommandLineOption jsonOption(QStringList() << "json"
                                                << "j",
                                  "Output results in JSON format");

    parser.addOption(typeOption);
    parser.addOption(methodOption);
    parser.addOption(queryOption);
    parser.addOption(caseSensitiveOption);
    parser.addOption(includeHiddenOption);
    parser.addOption(pinyinOption);
    parser.addOption(pinyinAcronymOption);
    parser.addOption(fileTypesOption);
    parser.addOption(fileExtensionsOption);
    parser.addOption(maxResultsOption);
    parser.addOption(maxPreviewOption);
    parser.addOption(wildcardOption);
    parser.addOption(jsonOption);

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
    } else if (queryStr == "wildcard" || parser.isSet(wildcardOption)) {
        queryType = SearchQuery::Type::Wildcard;
    } else if (queryStr != "simple") {
        std::cerr << "Error: Invalid query type. Use 'simple', 'boolean', or 'wildcard'" << std::endl;
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
    if (searchMethod == SearchMethod::Realtime) {
        options.setResultFoundEnabled(true);
        options.setDetailedResultsEnabled(true);
    }

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
        fileNameOptions.setPinyinAcronymEnabled(parser.isSet(pinyinAcronymOption));

        if (parser.isSet(fileTypesOption)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList types = parser.value(fileTypesOption).split(',', Qt::SkipEmptyParts);
#else
            QStringList types = parser.value(fileTypesOption).split(',', QString::SkipEmptyParts);
#endif
            fileNameOptions.setFileTypes(types);
        }

        if (parser.isSet(fileExtensionsOption)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            QStringList extensions = parser.value(fileExtensionsOption).split(',', Qt::SkipEmptyParts);
#else
            QStringList extensions = parser.value(fileExtensionsOption).split(',', QString::SkipEmptyParts);
#endif
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
    } else if (queryType == SearchQuery::Type::Wildcard) {
        query = SearchFactory::createQuery(keyword, SearchQuery::Type::Wildcard);
    } else {
        // For boolean query, split keywords by comma and create a boolean query
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        QStringList keywords = keyword.split(',', Qt::SkipEmptyParts);
#else
        QStringList keywords = keyword.split(',', QString::SkipEmptyParts);
#endif

        query = SearchFactory::createQuery(keywords, SearchQuery::Type::Boolean);
        query.setBooleanOperator(SearchQuery::BooleanOperator::AND);
    }

    // 检测是否使用 JSON 输出模式
    bool useJsonOutput = parser.isSet(jsonOption);

    // JSON 模式上下文（用于非流式模式）
    JsonOutputContext jsonContext;
    if (useJsonOutput && searchMethod == SearchMethod::Indexed) {
        // 非流式模式：收集所有结果后统一输出
        jsonContext.keyword = keyword;
        jsonContext.searchPath = searchPath;
        jsonContext.searchType = searchType;
        jsonContext.searchMethod = searchMethod;
        jsonContext.options = options;
        jsonContext.startTime = QDateTime::currentDateTime();
    }

    // Connect signals
    if (useJsonOutput) {
        // JSON 输出模式
        if (searchMethod == SearchMethod::Realtime) {
            // 流式 JSON 输出（实时搜索）
            QObject::connect(engine, &SearchEngine::searchStarted, [keyword, searchPath, searchType, searchMethod, &options]() {
                printJsonSearchStart(keyword, searchPath, searchType, searchMethod, options);
            });

            QObject::connect(engine, &SearchEngine::resultsFound, [searchType](const SearchResultList &results) {
                for (const auto &result : results)
                    printJsonResult(result, searchType);
            });

            // 记录搜索开始时间
            QDateTime searchStartTime = QDateTime::currentDateTime();

            QObject::connect(engine, &SearchEngine::searchFinished, [searchStartTime](const QList<SearchResult> &results) {
                qint64 elapsedMs = searchStartTime.msecsTo(QDateTime::currentDateTime());
                printJsonSearchEnd(results.size(), elapsedMs);
                QCoreApplication::quit();
            });

            QObject::connect(engine, &SearchEngine::searchCancelled, [] {
                QJsonObject cancelObj;
                cancelObj["type"] = "search_cancelled";
                cancelObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                printJsonLine(cancelObj);
                QCoreApplication::quit();
            });

            QObject::connect(engine, &SearchEngine::errorOccurred, [](const DFMSEARCH::SearchError &error) {
                QJsonObject errorObj;
                errorObj["type"] = "error";
                errorObj["name"] = error.name();
                errorObj["message"] = error.message();
                errorObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                printJsonLine(errorObj);
            });

        } else {
            // 完整 JSON 输出（索引搜索）
            QObject::connect(engine, &SearchEngine::searchStarted, []() {
                // 非流式模式：开始时不输出
            });

            QObject::connect(engine, &SearchEngine::resultsFound, [&jsonContext, searchType](const SearchResultList &results) {
                for (const auto &result : results) {
                    jsonContext.results.append(resultToJson(result, searchType));
                }
            });

            QObject::connect(engine, &SearchEngine::searchFinished, [&jsonContext, searchType](const QList<SearchResult> &results) {
                // 如果结果没有被 resultsFound 收集（禁用了 resultFoundEnabled），则在这里转换
                if (jsonContext.results.isEmpty() && !results.isEmpty()) {
                    for (const auto &result : results) {
                        jsonContext.results.append(resultToJson(result, searchType));
                    }
                }
                printJsonComplete(jsonContext);
                QCoreApplication::quit();
            });

            QObject::connect(engine, &SearchEngine::searchCancelled, [] {
                QJsonObject cancelObj;
                cancelObj["type"] = "search_cancelled";
                cancelObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                printJsonLine(cancelObj);
                QCoreApplication::quit();
            });

            QObject::connect(engine, &SearchEngine::errorOccurred, [](const DFMSEARCH::SearchError &error) {
                QJsonObject errorObj;
                errorObj["type"] = "error";
                errorObj["name"] = error.name();
                errorObj["message"] = error.message();
                errorObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
                printJsonLine(errorObj);
            });
        }
    } else {
        // 文本输出模式（原有逻辑）
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
    }

    engine->search(query);

    return a.exec();
}
