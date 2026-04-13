// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli_options.h"
#include "time_parser.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <iostream>

using namespace dfmsearch;
using namespace std;

CliOptions::CliOptions()
    : m_typeOption(QStringList() << "type", "Search type (filename, content or ocr)", "type", "filename")
    , m_methodOption(QStringList() << "method", "Search method (indexed or realtime)", "method", "indexed")
    , m_queryOption(QStringList() << "query", "Query type (simple, boolean or wildcard)", "query", "simple")
    , m_caseSensitiveOption(QStringList() << "case-sensitive", "Enable case sensitivity")
    , m_includeHiddenOption(QStringList() << "include-hidden", "Include hidden files")
    , m_pinyinOption(QStringList() << "pinyin", "Enable pinyin search (for filename search)")
    , m_pinyinAcronymOption(QStringList() << "pinyin-acronym", "Enable pinyin acronym search (for filename search)")
    , m_fileTypesOption(QStringList() << "file-types", "Filter by file types, comma separated", "types")
    , m_fileExtensionsOption(QStringList() << "file-extensions", "Filter by file extensions, comma separated", "extensions")
    , m_maxResultsOption(QStringList() << "max-results", "Maximum number of results", "number", "100")
    , m_maxPreviewOption(QStringList() << "max-preview", "Max content preview length", "length", "200")
    , m_wildcardOption(QStringList() << "wildcard", "Enable wildcard search with * and ? patterns")
    , m_jsonOption(QStringList() << "json"
                                 << "j",
                    "Output results in JSON format")
    , m_verboseOption(QStringList() << "verbose"
                                    << "v",
                      "Enable verbose output with detailed result information")
    , m_timeFieldOption(QStringList() << "time-field", "Time field to filter (birth or modify)", "field", "modify")
    , m_timeLastOption(QStringList() << "time-last", "Rolling time window (e.g., 3d, 2h, 30m)", "duration")
    , m_timeTodayOption(QStringList() << "time-today", "Filter files from today")
    , m_timeYesterdayOption(QStringList() << "time-yesterday", "Filter files from yesterday")
    , m_timeThisWeekOption(QStringList() << "time-this-week", "Filter files from this week")
    , m_timeLastWeekOption(QStringList() << "time-last-week", "Filter files from last week")
    , m_timeThisMonthOption(QStringList() << "time-this-month", "Filter files from this month")
    , m_timeLastMonthOption(QStringList() << "time-last-month", "Filter files from last month")
    , m_timeThisYearOption(QStringList() << "time-this-year", "Filter files from this year")
    , m_timeLastYearOption(QStringList() << "time-last-year", "Filter files from last year")
    , m_timeRangeOption(QStringList() << "time-range", "Custom time range (start,end)", "range")
{
    setupOptions();
}

void CliOptions::setupOptions()
{
    m_parser.setApplicationDescription("DFM Search Client");
    m_parser.addHelpOption();

    // 基本选项
    m_parser.addOption(m_typeOption);
    m_parser.addOption(m_methodOption);
    m_parser.addOption(m_queryOption);
    m_parser.addOption(m_caseSensitiveOption);
    m_parser.addOption(m_includeHiddenOption);
    m_parser.addOption(m_pinyinOption);
    m_parser.addOption(m_pinyinAcronymOption);
    m_parser.addOption(m_fileTypesOption);
    m_parser.addOption(m_fileExtensionsOption);
    m_parser.addOption(m_maxResultsOption);
    m_parser.addOption(m_maxPreviewOption);
    m_parser.addOption(m_wildcardOption);
    m_parser.addOption(m_jsonOption);
    m_parser.addOption(m_verboseOption);

    // 时间范围过滤选项
    m_parser.addOption(m_timeFieldOption);
    m_parser.addOption(m_timeLastOption);
    m_parser.addOption(m_timeTodayOption);
    m_parser.addOption(m_timeYesterdayOption);
    m_parser.addOption(m_timeThisWeekOption);
    m_parser.addOption(m_timeLastWeekOption);
    m_parser.addOption(m_timeThisMonthOption);
    m_parser.addOption(m_timeLastMonthOption);
    m_parser.addOption(m_timeThisYearOption);
    m_parser.addOption(m_timeLastYearOption);
    m_parser.addOption(m_timeRangeOption);

    // 位置参数
    m_parser.addPositionalArgument("keyword", "Search keyword");
    m_parser.addPositionalArgument("search_path", "Path to search in");
}

void CliOptions::printHelp() const
{
    std::cout << "Usage: dfm6-search-client [options] <keyword> <search_path>" << std::endl;
    std::cout << std::endl;
    std::cout << "Search Types:" << std::endl;
    std::cout << "  --type=<filename|content|ocr>  Search type (default: filename)" << std::endl;
    std::cout << "                                 filename: Search by file name" << std::endl;
    std::cout << "                                 content: Search by file content" << std::endl;
    std::cout << "                                 ocr: Search by OCR text in images" << std::endl;
    std::cout << std::endl;
    std::cout << "Search Options:" << std::endl;
    std::cout << "  --method=<indexed|realtime>    Search method (default: indexed)" << std::endl;
    std::cout << "  --query=<simple|boolean|wildcard> Query type (default: simple)" << std::endl;
    std::cout << "  --wildcard                     Enable wildcard search with * and ? patterns" << std::endl;
    std::cout << "  --case-sensitive               Enable case sensitivity" << std::endl;
    std::cout << "  --include-hidden               Include hidden files" << std::endl;
    std::cout << "  --pinyin                       Enable pinyin search (for filename search)" << std::endl;
    std::cout << "  --pinyin-acronym               Enable pinyin acronym search (for filename search)" << std::endl;
    std::cout << "  --file-types=<types>           Filter by file types, comma separated" << std::endl;
    std::cout << "  --file-extensions=<exts>       Filter by file extensions, comma separated" << std::endl;
    std::cout << "  --max-results=<number>         Maximum number of results" << std::endl;
    std::cout << "  --max-preview=<length>         Max content preview length (for content search)" << std::endl;
    std::cout << std::endl;
    std::cout << "Time Range Filter Options:" << std::endl;
    std::cout << "  --time-field=<birth|modify>    Time field to filter (birth=creation, modify=modification)" << std::endl;
    std::cout << "  --time-last=<N><unit>          Rolling time window: N units ago to now" << std::endl;
    std::cout << "                                 Units: m=minutes, h=hours, d=days, w=weeks, M=months, y=years" << std::endl;
    std::cout << "                                 Examples: --time-last=3d (last 3 days), --time-last=2h (last 2 hours)" << std::endl;
    std::cout << "  --time-today                   Filter files from today" << std::endl;
    std::cout << "  --time-yesterday               Filter files from yesterday" << std::endl;
    std::cout << "  --time-this-week               Filter files from this week" << std::endl;
    std::cout << "  --time-last-week               Filter files from last week" << std::endl;
    std::cout << "  --time-this-month              Filter files from this month" << std::endl;
    std::cout << "  --time-last-month              Filter files from last month" << std::endl;
    std::cout << "  --time-this-year               Filter files from this year" << std::endl;
    std::cout << "  --time-last-year               Filter files from last year" << std::endl;
    std::cout << "  --time-range=<start>,<end>     Custom time range (format: YYYY-MM-DD or \"YYYY-MM-DD HH:MM\")" << std::endl;
    std::cout << "                                 Example: --time-range=\"2025-01-01,2025-12-31\"" << std::endl;
    std::cout << std::endl;
    std::cout << "Output Options:" << std::endl;
    std::cout << "  --json, -j                     Output results in JSON format" << std::endl;
    std::cout << "  --verbose, -v                  Enable verbose output with detailed result information" << std::endl;
    std::cout << "  --help                         Display this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # Basic filename search" << std::endl;
    std::cout << "  dfm6-search-client \"document\" /home/user" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Content search" << std::endl;
    std::cout << "  dfm6-search-client --type=content \"hello world\" /home/user/Documents" << std::endl;
    std::cout << std::endl;
    std::cout << "  # OCR search in images" << std::endl;
    std::cout << "  dfm6-search-client --type=ocr \"screenshot\" /home/user/Pictures" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Realtime search with time filter" << std::endl;
    std::cout << "  dfm6-search-client --method=realtime --time-last=7d \"report\" /home/user" << std::endl;
}

bool CliOptions::parse(QCoreApplication &app, SearchCliConfig &config)
{
    m_parser.process(app);

    QStringList positionalArgs = m_parser.positionalArguments();
    if (positionalArgs.size() < 2) {
        printHelp();
        return false;
    }

    config.keyword = positionalArgs.at(0);
    config.searchPath = positionalArgs.at(1);

    // 验证搜索路径
    QFileInfo pathInfo(config.searchPath);
    if (!pathInfo.exists() || !pathInfo.isDir()) {
        std::cerr << "Error: Search path does not exist or is not a directory" << std::endl;
        return false;
    }

    // 解析搜索类型
    QString typeStr = m_parser.value(m_typeOption);
    if (typeStr == "content") {
        config.searchType = SearchType::Content;
    } else if (typeStr == "ocr") {
        config.searchType = SearchType::Ocr;
    } else if (typeStr != "filename") {
        std::cerr << "Error: Invalid search type. Use 'filename', 'content', or 'ocr'" << std::endl;
        return false;
    }

    // 解析搜索方法
    QString methodStr = m_parser.value(m_methodOption);
    if (methodStr == "realtime") {
        config.searchMethod = SearchMethod::Realtime;
    } else if (methodStr != "indexed") {
        std::cerr << "Error: Invalid search method. Use 'indexed' or 'realtime'" << std::endl;
        return false;
    }

    // 解析查询类型
    QString queryStr = m_parser.value(m_queryOption);
    if (queryStr == "boolean") {
        config.queryType = SearchQuery::Type::Boolean;
    } else if (queryStr == "wildcard" || m_parser.isSet(m_wildcardOption)) {
        config.queryType = SearchQuery::Type::Wildcard;
    } else if (queryStr != "simple") {
        std::cerr << "Error: Invalid query type. Use 'simple', 'boolean', or 'wildcard'" << std::endl;
        return false;
    }

    // 解析开关选项
    config.caseSensitive = m_parser.isSet(m_caseSensitiveOption);
    config.includeHidden = m_parser.isSet(m_includeHiddenOption);
    config.pinyinEnabled = m_parser.isSet(m_pinyinOption);
    config.pinyinAcronymEnabled = m_parser.isSet(m_pinyinAcronymOption);
    config.jsonOutput = m_parser.isSet(m_jsonOption);
    config.verbose = m_parser.isSet(m_verboseOption);

    // 解析过滤选项
    if (m_parser.isSet(m_fileTypesOption)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        config.fileTypes = m_parser.value(m_fileTypesOption).split(',', Qt::SkipEmptyParts);
#else
        config.fileTypes = m_parser.value(m_fileTypesOption).split(',', QString::SkipEmptyParts);
#endif
    }

    if (m_parser.isSet(m_fileExtensionsOption)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        config.fileExtensions = m_parser.value(m_fileExtensionsOption).split(',', Qt::SkipEmptyParts);
#else
        config.fileExtensions = m_parser.value(m_fileExtensionsOption).split(',', QString::SkipEmptyParts);
#endif
    }

    // 解析数值选项
    if (m_parser.isSet(m_maxResultsOption)) {
        bool ok;
        int maxResults = m_parser.value(m_maxResultsOption).toInt(&ok);
        if (ok && maxResults > 0) {
            config.maxResults = maxResults;
        }
    }

    if (m_parser.isSet(m_maxPreviewOption)) {
        bool ok;
        int previewLength = m_parser.value(m_maxPreviewOption).toInt(&ok);
        if (ok && previewLength > 0) {
            config.maxPreviewLength = previewLength;
        }
    }

    // 解析时间范围选项
    return parseTimeOptions(config);
}

bool CliOptions::parseTimeOptions(SearchCliConfig &config)
{
    // 设置时间字段
    QString timeFieldStr = m_parser.value(m_timeFieldOption);
    if (timeFieldStr == "birth") {
        config.timeFilter.setTimeField(DFMSEARCH::TimeField::BirthTime);
    } else if (timeFieldStr == "modify") {
        config.timeFilter.setTimeField(DFMSEARCH::TimeField::ModifyTime);
    } else {
        std::cerr << "Error: Invalid time field. Use 'birth' or 'modify'" << std::endl;
        return false;
    }

    // 处理时间范围选项（只能激活一个）
    if (m_parser.isSet(m_timeLastOption)) {
        QString lastArg = m_parser.value(m_timeLastOption);
        int value;
        DFMSEARCH::TimeUnit unit;
        if (TimeParser::parseTimeLast(lastArg, value, unit)) {
            config.timeFilter.setLast(value, unit);
            config.hasTimeFilter = true;
        } else {
            std::cerr << "Error: Invalid --time-last format. Use format like '3d', '2h', '30m'" << std::endl;
            return false;
        }
    } else if (m_parser.isSet(m_timeTodayOption)) {
        config.timeFilter.setToday();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeYesterdayOption)) {
        config.timeFilter.setYesterday();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeThisWeekOption)) {
        config.timeFilter.setThisWeek();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeLastWeekOption)) {
        config.timeFilter.setLastWeek();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeThisMonthOption)) {
        config.timeFilter.setThisMonth();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeLastMonthOption)) {
        config.timeFilter.setLastMonth();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeThisYearOption)) {
        config.timeFilter.setThisYear();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeLastYearOption)) {
        config.timeFilter.setLastYear();
        config.hasTimeFilter = true;
    } else if (m_parser.isSet(m_timeRangeOption)) {
        QString rangeArg = m_parser.value(m_timeRangeOption);
        QDateTime start, end;
        if (TimeParser::parseTimeRange(rangeArg, start, end)) {
            config.timeFilter.setRange(start, end);
            config.hasTimeFilter = true;
        } else {
            std::cerr << "Error: Invalid --time-range format. Use format 'YYYY-MM-DD,YYYY-MM-DD' or 'YYYY-MM-DD HH:MM,YYYY-MM-DD HH:MM'" << std::endl;
            return false;
        }
    }

    return true;
}
