// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef JSON_OUTPUT_H
#define JSON_OUTPUT_H

#include "output_formatter.h"
#include <dfm-search/searchoptions.h>

#include <QJsonArray>
#include <QDateTime>

namespace dfmsearch {

/**
 * @brief JSON格式输出器
 *
 * 支持流式和非流式两种JSON输出模式
 */
class JsonOutput : public OutputFormatter
{
    Q_OBJECT

public:
    explicit JsonOutput(bool streaming = false, QObject *parent = nullptr)
        : OutputFormatter(parent)
        , m_streaming(streaming) { }

    void setSearchContext(const QString &keyword, const QString &searchPath,
                          SearchType searchType, SearchMethod searchMethod) override;

    void outputSearchStarted() override;
    void outputResult(const SearchResult &result) override;
    void outputSearchFinished(const QList<SearchResult> &results) override;
    void outputSearchCancelled() override;
    void outputError(const DFMSEARCH::SearchError &error) override;

    /**
     * @brief 设置搜索选项
     */
    void setSearchOptions(const SearchOptions &options) { m_options = options; }

    /**
     * @brief 设置是否启用详细输出模式
     * @param verbose true 启用详细输出
     */
    void setVerbose(bool verbose) { m_verbose = verbose; }

private:
    QJsonValue resultToJson(const SearchResult &result);
    void printJsonLine(const QJsonObject &obj);

    // 流式输出方法
    void outputStreamingStart();
    void outputStreamingResult(const SearchResult &result);
    void outputStreamingFinish(const QList<SearchResult> &results);

    // 非流式输出方法
    void outputCompleteResult(const QList<SearchResult> &results);

private:
    QString m_keyword;
    QString m_searchPath;
    SearchType m_searchType = SearchType::FileName;
    SearchMethod m_searchMethod = SearchMethod::Indexed;
    SearchOptions m_options;
    QDateTime m_startTime;

    bool m_streaming;
    bool m_verbose = false;
    QJsonArray m_collectedResults;
};

}   // namespace dfmsearch

#endif   // JSON_OUTPUT_H
