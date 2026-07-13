// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli_options.h"
#include "time_parser.h"
#include "size_parser.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <iostream>
#include <cstdlib>

using namespace dfmsearch;
using namespace std;

CliOptions::CliOptions()
    : m_typeOption(QStringList() << "type",
                   "Search type (filename, content or ocr)", "type", "filename"),
      m_methodOption(QStringList() << "method", "Search method (indexed or realtime)", "method", "indexed"),
      m_queryOption(QStringList() << "query", "Query type (simple, boolean or wildcard)", "query", "simple"),
      m_caseSensitiveOption(QStringList() << "case-sensitive", "Enable case sensitivity"),
      m_includeHiddenOption(QStringList() << "include-hidden", "Include hidden files"),
      m_pinyinOption(QStringList() << "pinyin", "Enable pinyin search (for filename search)"),
      m_pinyinAcronymOption(QStringList() << "pinyin-acronym", "Enable pinyin acronym search (for filename search)"),
      m_fileTypesOption(QStringList() << "file-types", "Filter by file types, comma separated", "types"),
      m_fileExtensionsOption(QStringList() << "file-extensions", "Filter by file extensions, comma separated", "extensions"),
      m_excludeOption(QStringList() << "exclude", "Exclude directories from search, comma separated", "paths"),
      m_maxResultsOption(QStringList() << "max-results", "Maximum number of results (0 for unlimited)", "number", "0"),
      m_maxPreviewOption(QStringList() << "max-preview", "Max content preview length", "length", "200"),
      m_offsetOption(QStringList() << "offset", "Content offset: start reading from the n-th character (preview only)", "n", "0"),
      m_filenameOption(QStringList() << "filename", "Search by filename in content/ocr index", "keyword"),
      m_wildcardOption(QStringList() << "wildcard", "Enable wildcard search with * and ? patterns"),
      m_jsonOption(QStringList() << "json"
                                 << "j",
                   "Output results in JSON format"),
      m_verboseOption(QStringList() << "verbose"
                                    << "v",
                      "Enable verbose output with detailed result information"),
      m_semanticOption(QStringList() << "semantic"
                                     << "s",
                       "Enable semantic natural language search"),
      m_timeFieldOption(QStringList() << "time-field", "Time field to filter (birth or modify)", "field", "modify"),
      m_timeLastOption(QStringList() << "time-last", "Rolling time window (e.g., 3d, 2h, 30m)", "duration"),
      m_timeTodayOption(QStringList() << "time-today", "Filter files from today"),
      m_timeYesterdayOption(QStringList() << "time-yesterday", "Filter files from yesterday"),
      m_timeThisWeekOption(QStringList() << "time-this-week", "Filter files from this week"),
      m_timeLastWeekOption(QStringList() << "time-last-week", "Filter files from last week"),
      m_timeThisMonthOption(QStringList() << "time-this-month", "Filter files from this month"),
      m_timeLastMonthOption(QStringList() << "time-last-month", "Filter files from last month"),
      m_timeThisYearOption(QStringList() << "time-this-year", "Filter files from this year"),
      m_timeLastYearOption(QStringList() << "time-last-year", "Filter files from last year"),
      m_timeRangeOption(QStringList() << "time-range", "Custom time range (start,end)", "range"),
      m_sizeMinOption(QStringList() << "size-min", "Minimum file size (e.g., 1K, 10M, 1G, 512)", "size"),
      m_sizeMaxOption(QStringList() << "size-max", "Maximum file size (e.g., 1K, 10M, 1G, 512)", "size"),
      m_versionOption(QStringList() << "version", "Show version information and exit")
{
    setupOptions();
}

void CliOptions::setupOptions()
{
    m_parser.setApplicationDescription("DFM Search Client");
    m_parser.addHelpOption();
    m_parser.addOption(m_versionOption);

    // Basic options
    m_parser.addOption(m_typeOption);
    m_parser.addOption(m_methodOption);
    m_parser.addOption(m_queryOption);
    m_parser.addOption(m_caseSensitiveOption);
    m_parser.addOption(m_includeHiddenOption);
    m_parser.addOption(m_pinyinOption);
    m_parser.addOption(m_pinyinAcronymOption);
    m_parser.addOption(m_fileTypesOption);
    m_parser.addOption(m_fileExtensionsOption);
    m_parser.addOption(m_excludeOption);
    m_parser.addOption(m_maxResultsOption);
    m_parser.addOption(m_maxPreviewOption);
    m_parser.addOption(m_offsetOption);
    m_parser.addOption(m_filenameOption);
    m_parser.addOption(m_wildcardOption);
    m_parser.addOption(m_jsonOption);
    m_parser.addOption(m_verboseOption);
    m_parser.addOption(m_semanticOption);

    // Time range filtering options
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

    // File size range filtering options
    m_parser.addOption(m_sizeMinOption);
    m_parser.addOption(m_sizeMaxOption);

    // Positional arguments
    m_parser.addPositionalArgument("keyword", "Search keyword");
    m_parser.addPositionalArgument("search_path", "Path to search in");
}

void CliOptions::printHelp() const
{
    std::cout << "Usage: dfm-searcher [options] <keyword> [search_path]" << std::endl;
    std::cout << std::endl;
    std::cout << "Semantic Search:" << std::endl;
    std::cout << "  --semantic, -s                 Enable semantic natural language search" << std::endl;
    std::cout << "                                 Example: dfm-searcher -s \"recent 3 days images\" /home/user" << std::endl;
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
    std::cout << "                                 boolean: Separate keywords with | for OR, & or , for AND" << std::endl;
    std::cout << "  --wildcard                     Enable wildcard search with * and ? patterns" << std::endl;
    std::cout << "  --case-sensitive               Enable case sensitivity" << std::endl;
    std::cout << "  --include-hidden               Include hidden files" << std::endl;
    std::cout << "  --pinyin                       Enable pinyin search (for filename search)" << std::endl;
    std::cout << "  --pinyin-acronym               Enable pinyin acronym search (for filename search)" << std::endl;
    std::cout << "  --file-types=<types>           Filter by file types, comma separated" << std::endl;
    std::cout << "  --file-extensions=<exts>       Filter by file extensions, comma separated" << std::endl;
    std::cout << "  --exclude=<paths>              Exclude directories from search, comma separated" << std::endl;
    std::cout << "  --max-results=<number>         Maximum number of results (0 for unlimited)" << std::endl;
    std::cout << "  --max-preview=<length>         Max content preview length (for content/ocr search)" << std::endl;
    std::cout << "  --offset=<n>                   Content offset: start reading from the n-th character (preview only, default 0)" << std::endl;
    std::cout << "  --filename=<keyword>           Search by filename in content/ocr index" << std::endl;
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
    std::cout << "File Size Range Filter Options:" << std::endl;
    std::cout << "  --size-min=<size>              Minimum file size (e.g., 1K, 10M, 1G, 512)" << std::endl;
    std::cout << "                                 Units: K=KB, M=MB, G=GB, T=TB (default: bytes)" << std::endl;
    std::cout << "  --size-max=<size>              Maximum file size (e.g., 1K, 10M, 1G, 512)" << std::endl;
    std::cout << "                                 Units: K=KB, M=MB, G=GB, T=TB (default: bytes)" << std::endl;
    std::cout << "                                 Example: --size-min=1M --size-max=100M" << std::endl;
    std::cout << std::endl;
    std::cout << "Output Options:" << std::endl;
    std::cout << "  --json, -j                     Output results in JSON format" << std::endl;
    std::cout << "  --verbose, -v                  Enable verbose output with detailed result information" << std::endl;
    std::cout << "  --help                         Display this help" << std::endl;
    std::cout << "  --version                      Show version information and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # Basic filename search" << std::endl;
    std::cout << "  dfm-searcher \"document\" /home/user" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Content search" << std::endl;
    std::cout << "  dfm-searcher --type=content \"hello world\" /home/user/Documents" << std::endl;
    std::cout << std::endl;
    std::cout << "  # OCR search in images" << std::endl;
    std::cout << "  dfm-searcher --type=ocr \"screenshot\" /home/user/Pictures" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Realtime search with time filter" << std::endl;
    std::cout << "  dfm-searcher --method=realtime --time-last=7d \"report\" /home/user" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Semantic search: find recent images" << std::endl;
    std::cout << "  dfm-searcher --semantic \"recent 3 days images\" /home/user" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Semantic search with JSON output" << std::endl;
    std::cout << "  dfm-searcher -s -j \"content contains meeting notes\" /home/user" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Filename search with file size filter (1MB to 100MB)" << std::endl;
    std::cout << "  dfm-searcher --size-min=1M --size-max=100M \"video\" /home/user" << std::endl;
    std::cout << std::endl;
    std::cout << "Content Preview (on-demand):" << std::endl;
    std::cout << "  dfm-searcher preview [<keyword>] <path1> [path2 ...] [--type=<content|ocr>] [--offset=<n>] [--max-preview=<length>] [-j]" << std::endl;
    std::cout << "  Fetch content snippets for specific files without running a full search." << std::endl;
    std::cout << "  With keyword: returns snippet from the keyword match position (search starts at --offset)." << std::endl;
    std::cout << "  Without keyword: returns exact content starting from --offset (full content if --max-preview is omitted)." << std::endl;
    std::cout << "  Search type is auto-detected by file extension; use --type to force a specific index." << std::endl;
    std::cout << std::endl;
    std::cout << "  # Fetch highlighted snippet for a single file" << std::endl;
    std::cout << "  dfm-searcher preview \"hello\" /home/user/doc.txt" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Fetch full content (no keyword)" << std::endl;
    std::cout << "  dfm-searcher preview /home/user/doc.txt" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Read content from offset 100, 50 chars max" << std::endl;
    std::cout << "  dfm-searcher preview /home/user/doc.txt --offset=100 --max-preview=50 -j" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Search keyword from offset 150" << std::endl;
    std::cout << "  dfm-searcher preview \"keyword\" /home/user/doc.txt --offset=150 -j" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Batch fetch highlighted snippets with JSON output" << std::endl;
    std::cout << "  dfm-searcher preview \"screenshot\" img1.png img2.png -j" << std::endl;
    std::cout << std::endl;
    std::cout << "  # Force a specific search type" << std::endl;
    std::cout << "  dfm-searcher preview --type=ocr \"screenshot\" photo.png" << std::endl;
}

bool CliOptions::parse(QCoreApplication &app, SearchCliConfig &config)
{
    // Pre-scan for "preview" subcommand
    const QStringList rawArgs = app.arguments();
    // Intercept -h/--help to show custom help (includes preview subcommand docs)
    if (rawArgs.contains("-h") || rawArgs.contains("--help")) {
        printHelp();
        return false;
    }
    if (rawArgs.size() >= 2 && rawArgs.at(1) == "preview") {
        config.subcommand = "preview";
    }

    m_parser.process(app);

    if (m_parser.isSet(m_versionOption)) {
        QString name = QCoreApplication::applicationName().isEmpty()
            ? QStringLiteral("dfm-searcher")
            : QCoreApplication::applicationName();
        std::cout << name.toStdString() << " "
                  << QCoreApplication::applicationVersion().toStdString() << std::endl;
        std::exit(0);
    }

    QStringList positionalArgs = m_parser.positionalArguments();

    // For preview subcommand, the positional args are: [<keyword>] <path1> [path2 ...]
    // "preview" itself is consumed as first positional by QCommandLineParser
    if (config.subcommand == "preview") {
        // Skip "preview" keyword from positional args (it was parsed as the first positional)
        QStringList args = positionalArgs;
        if (!args.isEmpty() && args.first() == "preview") {
            args.removeFirst();
        }
        if (args.isEmpty()) {
            std::cerr << "Error: preview requires at least one <path>" << std::endl;
            return false;
        }

        // If first arg is an existing file, treat all args as paths (no-keyword mode).
        // Otherwise first arg is the keyword and the rest are paths.
        if (QFileInfo::exists(args.first())) {
            config.keyword.clear();
            config.searchPath = args.join(',');
        } else {
            config.keyword = args.first();
            config.searchPath = args.mid(1).join(',');
            if (config.searchPath.isEmpty()) {
                std::cerr << "Error: preview with keyword requires at least one <path>" << std::endl;
                return false;
            }
        }

        // Search type: default Semantic (auto-detect by extension), optional override
        QString typeStr = m_parser.value(m_typeOption);
        if (typeStr == "content") {
            config.searchType = SearchType::Content;
        } else if (typeStr == "ocr") {
            config.searchType = SearchType::Ocr;
        } else {
            config.searchType = SearchType::Semantic;
        }

        config.jsonOutput = m_parser.isSet(m_jsonOption);
        if (m_parser.isSet(m_maxPreviewOption)) {
            bool ok;
            int previewLength = m_parser.value(m_maxPreviewOption).toInt(&ok);
            if (ok && previewLength > 0) {
                config.maxPreviewLength = previewLength;
            }
            config.maxPreviewSet = true;
        }
        if (m_parser.isSet(m_offsetOption)) {
            bool ok;
            int offsetVal = m_parser.value(m_offsetOption).toInt(&ok);
            if (ok && offsetVal >= 0) {
                config.offset = offsetVal;
            }
        }
        return true;
    }

    if (positionalArgs.isEmpty()) {
        printHelp();
        return false;
    }

    // Semantic mode: only keyword is required, search path is optional
    config.semanticMode = m_parser.isSet(m_semanticOption);
    config.keyword = positionalArgs.at(0);
    if (positionalArgs.size() >= 2) {
        config.searchPath = positionalArgs.at(1);
    }

    // Validate search path (not required in semantic mode)
    if (!config.searchPath.isEmpty()) {
        QFileInfo pathInfo(config.searchPath);
        if (!pathInfo.exists() || !pathInfo.isDir()) {
            std::cerr << "Error: Search path does not exist or is not a directory" << std::endl;
            return false;
        }
    } else if (!config.semanticMode) {
        std::cerr << "Error: Search path is required" << std::endl;
        printHelp();
        return false;
    }

    // Auto-enable includeHidden when search path contains hidden directory components.
    // User who explicitly specifies a hidden path (e.g. ~/.local/share/Trash)
    // expects results without needing --include-hidden.
    config.includeHidden = m_parser.isSet(m_includeHiddenOption);
    if (!config.includeHidden && !config.searchPath.isEmpty()
        && Global::isHiddenPathOrInHiddenDir(config.searchPath)) {
        config.includeHidden = true;
    }

    // In semantic mode, skip type/method/query parsing
    if (config.semanticMode) {
        config.jsonOutput = m_parser.isSet(m_jsonOption);
        config.verbose = m_parser.isSet(m_verboseOption);
        if (m_parser.isSet(m_maxPreviewOption)) {
            bool ok;
            int previewLength = m_parser.value(m_maxPreviewOption).toInt(&ok);
            if (ok && previewLength > 0) {
                config.maxPreviewLength = previewLength;
            }
        }
        if (m_parser.isSet(m_maxResultsOption)) {
            bool ok;
            int maxResults = m_parser.value(m_maxResultsOption).toInt(&ok);
            if (ok && maxResults >= 0) {
                config.maxResults = maxResults;
            }
        }
        if (m_parser.isSet(m_excludeOption)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            config.excludedPaths = m_parser.value(m_excludeOption).split(',', Qt::SkipEmptyParts);
#else
            config.excludedPaths = m_parser.value(m_excludeOption).split(',', QString::SkipEmptyParts);
#endif
        }
        return true;
    }

    // Parse search type (non-semantic mode only)
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

    if (m_parser.isSet(m_excludeOption)) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        config.excludedPaths = m_parser.value(m_excludeOption).split(',', Qt::SkipEmptyParts);
#else
        config.excludedPaths = m_parser.value(m_excludeOption).split(',', QString::SkipEmptyParts);
#endif
    }

    // 解析文件名搜索选项（仅对 content/ocr 搜索有效）
    if (m_parser.isSet(m_filenameOption)) {
        config.filenameKeyword = m_parser.value(m_filenameOption);
    }

    // 解析数值选项
    if (m_parser.isSet(m_maxResultsOption)) {
        bool ok;
        int maxResults = m_parser.value(m_maxResultsOption).toInt(&ok);
        if (ok && maxResults >= 0) {
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
    if (!parseTimeOptions(config)) {
        return false;
    }

    // 解析文件大小范围选项
    if (m_parser.isSet(m_sizeMinOption)) {
        qint64 minBytes = 0;
        if (SizeParser::parseSize(m_parser.value(m_sizeMinOption), minBytes) && minBytes > 0) {
            config.sizeFilter.setMin(minBytes);
            config.hasSizeFilter = true;
        } else {
            std::cerr << "Error: Invalid --size-min format. Use format like '1K', '10M', '1G', or '512'" << std::endl;
            return false;
        }
    }

    if (m_parser.isSet(m_sizeMaxOption)) {
        qint64 maxBytes = 0;
        if (SizeParser::parseSize(m_parser.value(m_sizeMaxOption), maxBytes) && maxBytes > 0) {
            config.sizeFilter.setMax(maxBytes);
            config.hasSizeFilter = true;
        } else {
            std::cerr << "Error: Invalid --size-max format. Use format like '1K', '10M', '1G', or '512'" << std::endl;
            return false;
        }
    }

    // Validate option compatibility with search type
    if (config.searchType == SearchType::FileName) {
        if (m_parser.isSet(m_filenameOption)) {
            std::cerr << "Error: --filename is only valid for content/ocr search, not filename search" << std::endl;
            return false;
        }
    } else {
        if (m_parser.isSet(m_pinyinOption)) {
            std::cerr << "Error: --pinyin is only valid for filename search" << std::endl;
            return false;
        }
        if (m_parser.isSet(m_pinyinAcronymOption)) {
            std::cerr << "Error: --pinyin-acronym is only valid for filename search" << std::endl;
            return false;
        }
        if (m_parser.isSet(m_fileTypesOption)) {
            std::cerr << "Error: --file-types is only valid for filename search" << std::endl;
            return false;
        }
    }

    return true;
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
