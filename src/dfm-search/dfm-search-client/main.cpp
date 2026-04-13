// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QDebug>

#include <dfm-search/searchengine.h>
#include <dfm-search/searchfactory.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/ocrtextsearchapi.h>

#include "cli_options.h"
#include "output/text_output.h"
#include "output/json_output.h"

using namespace dfmsearch;

/**
 * @brief 配置搜索引擎选项
 */
static void configureSearchOptions(SearchOptions &options, const SearchCliConfig &config)
{
    options.setSearchMethod(config.searchMethod);
    options.setCaseSensitive(config.caseSensitive);
    options.setIncludeHidden(config.includeHidden);
    options.setSearchPath(config.searchPath);
    options.setMaxResults(config.maxResults);
    options.setDetailedResultsEnabled(true);

    if (config.searchMethod == SearchMethod::Realtime) {
        options.setResultFoundEnabled(true);
    }

    // 配置类型特定选项
    if (config.searchType == SearchType::FileName) {
        FileNameOptionsAPI fileNameOptions(options);
        fileNameOptions.setPinyinEnabled(config.pinyinEnabled);
        fileNameOptions.setPinyinAcronymEnabled(config.pinyinAcronymEnabled);

        if (!config.fileTypes.isEmpty()) {
            fileNameOptions.setFileTypes(config.fileTypes);
        }
        if (!config.fileExtensions.isEmpty()) {
            fileNameOptions.setFileExtensions(config.fileExtensions);
        }
    } else if (config.searchType == SearchType::Content) {
        ContentOptionsAPI contentOptions(options);
        contentOptions.setMaxPreviewLength(config.maxPreviewLength);
        contentOptions.setFullTextRetrievalEnabled(true);
        contentOptions.setSearchResultHighlightEnabled(true);
        contentOptions.setFilenameContentMixedAndSearchEnabled(true);
    } else if (config.searchType == SearchType::Ocr) {
        OcrTextOptionsAPI ocrTextOptions(options);
        ocrTextOptions.setFilenameOcrContentMixedAndSearchEnabled(true);
    }

    // 应用时间范围过滤
    if (config.hasTimeFilter) {
        options.setTimeRangeFilter(config.timeFilter);
    }
}

/**
 * @brief 创建输出格式化器
 */
static OutputFormatter *createOutputFormatter(const SearchCliConfig &config, QObject *parent)
{
    if (config.jsonOutput) {
        // JSON 输出：实时搜索使用流式，索引搜索使用完整输出
        bool streaming = (config.searchMethod == SearchMethod::Realtime);
        return new JsonOutput(streaming, parent);
    }
    return new TextOutput(parent);
}

/**
 * @brief 连接搜索引擎信号到输出格式化器
 */
static void connectSignals(SearchEngine *engine, OutputFormatter *formatter,
                           const SearchCliConfig &config, QCoreApplication &app)
{
    // 连接 finished 信号以退出应用
    QObject::connect(formatter, &OutputFormatter::finished, &app, &QCoreApplication::quit);

    // 搜索开始
    QObject::connect(engine, &SearchEngine::searchStarted, [formatter]() {
        formatter->outputSearchStarted();
    });

    // 结果到达
    QObject::connect(engine, &SearchEngine::resultsFound, [formatter](const SearchResultList &results) {
        for (const auto &result : results) {
            formatter->outputResult(result);
        }
    });

    // 搜索完成
    QObject::connect(engine, &SearchEngine::searchFinished, [formatter](const QList<SearchResult> &results) {
        formatter->outputSearchFinished(results);
    });

    // 搜索取消
    QObject::connect(engine, &SearchEngine::searchCancelled, [formatter]() {
        formatter->outputSearchCancelled();
    });

    // 错误处理
    QObject::connect(engine, &SearchEngine::errorOccurred, [formatter](const DFMSEARCH::SearchError &error) {
        formatter->outputError(error);
    });
}

/**
 * @brief 创建搜索查询
 */
static SearchQuery createSearchQuery(const SearchCliConfig &config)
{
    if (config.queryType == SearchQuery::Type::Simple) {
        return SearchFactory::createQuery(config.keyword, SearchQuery::Type::Simple);
    } else if (config.queryType == SearchQuery::Type::Wildcard) {
        return SearchFactory::createQuery(config.keyword, SearchQuery::Type::Wildcard);
    } else {
        // Boolean 查询：按逗号分割关键字
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        QStringList keywords = config.keyword.split(',', Qt::SkipEmptyParts);
#else
        QStringList keywords = config.keyword.split(',', QString::SkipEmptyParts);
#endif
        SearchQuery query = SearchFactory::createQuery(keywords, SearchQuery::Type::Boolean);
        query.setBooleanOperator(SearchQuery::BooleanOperator::AND);
        return query;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 解析命令行参数
    CliOptions cliOptions;
    SearchCliConfig config;
    if (!cliOptions.parse(app, config)) {
        return 1;
    }

    // 创建搜索引擎
    SearchEngine *engine = SearchFactory::createEngine(config.searchType, &app);
    if (!engine) {
        qCritical() << "Error: Failed to create search engine";
        return 1;
    }

    // 配置搜索选项
    SearchOptions options;
    configureSearchOptions(options, config);
    engine->setSearchOptions(options);

    // 创建输出格式化器
    OutputFormatter *formatter = createOutputFormatter(config, &app);

    // 设置输出格式化器上下文
    formatter->setSearchContext(config.keyword, config.searchPath,
                                config.searchType, config.searchMethod);

    // 为文本输出设置额外信息
    TextOutput *textOutput = qobject_cast<TextOutput *>(formatter);
    if (textOutput) {
        textOutput->setSearchOptions(options);
        textOutput->setTimeFilterInfo(config.hasTimeFilter, config.timeFilter);
        if (!config.fileExtensions.isEmpty()) {
            textOutput->setFileExtensionsFilter(config.fileExtensions.join(','));
        }
    }

    // 为JSON输出设置额外信息
    JsonOutput *jsonOutput = qobject_cast<JsonOutput *>(formatter);
    if (jsonOutput) {
        jsonOutput->setSearchOptions(options);
    }

    // 连接信号
    connectSignals(engine, formatter, config, app);

    // 创建并执行搜索查询
    SearchQuery query = createSearchQuery(config);
    engine->search(query);

    return app.exec();
}
