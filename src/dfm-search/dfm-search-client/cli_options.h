// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CLI_OPTIONS_H
#define CLI_OPTIONS_H

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStringList>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/timerangefilter.h>

namespace dfmsearch {

/**
 * @brief 命令行搜索参数配置
 *
 * 封装所有命令行解析结果，遵循单一职责原则
 */
struct SearchCliConfig
{
    // 基本参数
    QString keyword;
    QString searchPath;
    SearchType searchType = SearchType::FileName;
    SearchMethod searchMethod = SearchMethod::Indexed;
    SearchQuery::Type queryType = SearchQuery::Type::Simple;

    // 开关选项
    bool caseSensitive = false;
    bool includeHidden = false;
    bool pinyinEnabled = false;
    bool pinyinAcronymEnabled = false;
    bool wildcardEnabled = false;
    bool jsonOutput = false;
    bool verbose = false;   // 详细输出模式

    // 过滤选项
    QStringList fileTypes;
    QStringList fileExtensions;
    int maxResults = 100;
    int maxPreviewLength = 200;

    // 时间范围过滤
    bool hasTimeFilter = false;
    DFMSEARCH::TimeRangeFilter timeFilter;
};

/**
 * @brief 命令行选项管理器
 *
 * 负责定义和解析命令行参数，遵循单一职责原则
 */
class CliOptions
{
public:
    CliOptions();
    ~CliOptions() = default;

    /**
     * @brief 解析命令行参数
     * @param app QCoreApplication实例
     * @param config 输出的配置结构
     * @return 解析成功返回true，失败返回false
     */
    bool parse(QCoreApplication &app, SearchCliConfig &config);

    /**
     * @brief 打印帮助信息
     */
    void printHelp() const;

private:
    void setupOptions();
    bool validateConfig(SearchCliConfig &config);
    bool parseTimeOptions(SearchCliConfig &config);

private:
    QCommandLineParser m_parser;

    // 基本选项
    QCommandLineOption m_typeOption;
    QCommandLineOption m_methodOption;
    QCommandLineOption m_queryOption;
    QCommandLineOption m_caseSensitiveOption;
    QCommandLineOption m_includeHiddenOption;
    QCommandLineOption m_pinyinOption;
    QCommandLineOption m_pinyinAcronymOption;
    QCommandLineOption m_fileTypesOption;
    QCommandLineOption m_fileExtensionsOption;
    QCommandLineOption m_maxResultsOption;
    QCommandLineOption m_maxPreviewOption;
    QCommandLineOption m_wildcardOption;
    QCommandLineOption m_jsonOption;
    QCommandLineOption m_verboseOption;

    // 时间范围过滤选项
    QCommandLineOption m_timeFieldOption;
    QCommandLineOption m_timeLastOption;
    QCommandLineOption m_timeTodayOption;
    QCommandLineOption m_timeYesterdayOption;
    QCommandLineOption m_timeThisWeekOption;
    QCommandLineOption m_timeLastWeekOption;
    QCommandLineOption m_timeThisMonthOption;
    QCommandLineOption m_timeLastMonthOption;
    QCommandLineOption m_timeThisYearOption;
    QCommandLineOption m_timeLastYearOption;
    QCommandLineOption m_timeRangeOption;
};

}   // namespace dfmsearch

#endif   // CLI_OPTIONS_H
