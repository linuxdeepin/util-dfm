// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>

#include <dfm-search/searchengine.h>
#include <dfm-search/searchfactory.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/ocrtextsearchapi.h>
#include <dfm-search/semanticsearcher.h>
#include <dfm-search/contentretriever.h>

#include "cli_options.h"
#include "output/text_output.h"
#include "output/json_output.h"

#include <iostream>

#ifndef VERSION
#define VERSION "unknown"
#endif

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
    options.setDetailedResultsEnabled(config.verbose);   // 使用 verbose 选项控制详细输出

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
        contentOptions.setFullTextRetrievalEnabled(config.verbose);
        contentOptions.setSearchResultHighlightEnabled(config.verbose);
        contentOptions.setFilenameContentMixedAndSearchEnabled(true);
        if (!config.filenameKeyword.isEmpty()) {
            contentOptions.setFilenameKeyword(config.filenameKeyword);
        }
        if (!config.fileExtensions.isEmpty()) {
            contentOptions.setFileExtensions(config.fileExtensions);
        }
    } else if (config.searchType == SearchType::Ocr) {
        OcrTextOptionsAPI ocrTextOptions(options);
        ocrTextOptions.setMaxPreviewLength(config.maxPreviewLength);
        ocrTextOptions.setFullTextRetrievalEnabled(config.verbose);
        ocrTextOptions.setSearchResultHighlightEnabled(config.verbose);
        ocrTextOptions.setFilenameOcrContentMixedAndSearchEnabled(true);
        if (!config.filenameKeyword.isEmpty()) {
            ocrTextOptions.setFilenameKeyword(config.filenameKeyword);
        }
        if (!config.fileExtensions.isEmpty()) {
            ocrTextOptions.setFileExtensions(config.fileExtensions);
        }
    }

    // 应用时间范围过滤
    if (config.hasTimeFilter) {
        options.setTimeRangeFilter(config.timeFilter);
    }

    // 应用文件大小范围过滤
    if (config.hasSizeFilter) {
        options.setSizeRangeFilter(config.sizeFilter);
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
        // Boolean 查询：根据分隔符确定 AND/OR 逻辑
        // '|' 分隔 → OR，'&' 分隔 → AND，',' 分隔 → AND（向后兼容）
        QChar separator = ',';
        SearchQuery::BooleanOperator op = SearchQuery::BooleanOperator::AND;
        if (config.keyword.contains('|')) {
            separator = '|';
            op = SearchQuery::BooleanOperator::OR;
        } else if (config.keyword.contains('&')) {
            separator = '&';
        }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        QStringList keywords = config.keyword.split(separator, Qt::SkipEmptyParts);
#else
        QStringList keywords = config.keyword.split(separator, QString::SkipEmptyParts);
#endif
        SearchQuery query = SearchFactory::createQuery(keywords, SearchQuery::Type::Boolean);
        query.setBooleanOperator(op);
        return query;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationVersion(QString::fromLatin1(VERSION));

    // Parse CLI arguments
    CliOptions cliOptions;
    SearchCliConfig config;
    if (!cliOptions.parse(app, config)) {
        return 1;
    }

    // Preview subcommand: fetch content preview on demand
    if (config.subcommand == "preview") {
        DFMSEARCH::ContentRetriever retriever;
        DFMSEARCH::PreviewOptions previewOptions;
        previewOptions.setOffset(config.offset);
        // 无 keyword 且未显式设置 --max-preview：maxLength=0 表示无限制（返回全文）
        // 有 keyword 或显式设置了 --max-preview：使用 maxPreviewLength 值
        if (config.keyword.isEmpty() && !config.maxPreviewSet) {
            previewOptions.setMaxLength(0);
        } else {
            previewOptions.setMaxLength(config.maxPreviewLength);
        }
        previewOptions.setKeyword(config.keyword);

        // Paths are stored as comma-separated in config.searchPath
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        QStringList paths = config.searchPath.split(',', Qt::SkipEmptyParts);
#else
        QStringList paths = config.searchPath.split(',', QString::SkipEmptyParts);
#endif

        if (config.jsonOutput) {
            // JSON output
            QJsonObject root;
            root["type"] = "preview";
            root["searchType"] = (config.searchType == SearchType::Content) ? "content"
                : (config.searchType == SearchType::Ocr) ? "ocr" : "semantic";
            root["keyword"] = config.keyword;
            root["offset"] = config.offset;
            root["maxLength"] = previewOptions.maxLength();

            QJsonArray results;
            for (const QString &path : paths) {
                QJsonObject item;
                item["path"] = path;
                DFMSEARCH::PreviewResult result = retriever.fetchPreview(path, config.searchType, previewOptions);
                item["content"] = result.content();
                item["keywordOffset"] = result.keywordOffset();
                results.append(item);
            }

            root["totalResults"] = results.size();
            root["results"] = results;

            QJsonDocument doc(root);
            std::cout << doc.toJson(QJsonDocument::Indented).toStdString() << std::endl;
        } else {
            // Text output
            QTextStream out(stdout);
            for (const QString &path : paths) {
                out << path << "\n";
                DFMSEARCH::PreviewResult result = retriever.fetchPreview(path, config.searchType, previewOptions);
                if (!result.content().isEmpty()) {
                    out << "  " << result.content() << "\n";
                } else {
                    out << "  (no content)\n";
                }
                out << Qt::endl;
            }
        }
        return 0;
    }

    // Semantic search mode
    if (config.semanticMode) {
        auto *semanticSearcher = new DFMSEARCH::SemanticSearcher(&app);
        semanticSearcher->setDetailedResultsEnabled(config.verbose);
        if (config.maxResults > 0) {
            semanticSearcher->setMaxResults(config.maxResults);
        }

        OutputFormatter *formatter = createOutputFormatter(config, &app);

        // 为语义搜索构建 formatter 需要的 options
        SearchOptions formatterOptions;
        formatterOptions.setDetailedResultsEnabled(config.verbose);
        if (config.hasTimeFilter) formatterOptions.setTimeRangeFilter(config.timeFilter);
        if (config.hasSizeFilter) formatterOptions.setSizeRangeFilter(config.sizeFilter);

        JsonOutput *jsonOutput = qobject_cast<JsonOutput *>(formatter);
        if (jsonOutput) {
            jsonOutput->setSearchOptions(formatterOptions);
        }
        TextOutput *textOutput = qobject_cast<TextOutput *>(formatter);
        if (textOutput) {
            textOutput->setSearchOptions(formatterOptions);
            textOutput->setVerbose(config.verbose);
        }

        formatter->setSearchContext(config.keyword, config.searchPath,
                                    SearchType::Semantic, SearchMethod::Indexed);

        QObject::connect(formatter, &OutputFormatter::finished, &app, &QCoreApplication::quit);
        QObject::connect(semanticSearcher, &DFMSEARCH::SemanticSearcher::intentParsed,
                         [formatter](const DFMSEARCH::ParsedIntent &intent) {
                             if (auto *jsonOut = qobject_cast<JsonOutput *>(formatter)) {
                                 jsonOut->setParsedIntent(intent);
                             }
                         });
        QObject::connect(semanticSearcher, &DFMSEARCH::SemanticSearcher::searchStarted, [formatter]() {
            formatter->outputSearchStarted();
        });
        QObject::connect(semanticSearcher, &DFMSEARCH::SemanticSearcher::searchFinished, [formatter](const SearchResultList &results) {
            formatter->outputSearchFinished(results);
        });
        QObject::connect(semanticSearcher, &DFMSEARCH::SemanticSearcher::searchCancelled, [formatter]() {
            formatter->outputSearchCancelled();
        });
        QObject::connect(semanticSearcher, &DFMSEARCH::SemanticSearcher::errorOccurred, [formatter](const DFMSEARCH::SearchError &error) {
            formatter->outputError(error);
        });

        QStringList semanticDirs;
        if (!config.searchPath.isEmpty()) {
            semanticDirs = QStringList { config.searchPath };
        }
        semanticSearcher->search(config.keyword, semanticDirs);
        return app.exec();
    }

    // Create search engine (non-semantic mode)
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
        textOutput->setVerbose(config.verbose);
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
