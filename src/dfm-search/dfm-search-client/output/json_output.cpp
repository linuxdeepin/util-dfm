// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "json_output.h"
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/ocrtextsearchapi.h>

#include <QJsonDocument>
#include <iostream>

using namespace dfmsearch;
using namespace std;

void JsonOutput::setSearchContext(const QString &keyword, const QString &searchPath,
                                  SearchType searchType, SearchMethod searchMethod)
{
    m_keyword = keyword;
    m_searchPath = searchPath;
    m_searchType = searchType;
    m_searchMethod = searchMethod;
}

QJsonValue JsonOutput::resultToJson(const SearchResult &result)
{
    // 如果不启用详细结果，只返回路径
    if (!m_options.detailedResultsEnabled()) {
        return result.path();
    }

    if (m_searchType == SearchType::FileName) {
        // 文件名搜索：返回详细对象
        QJsonObject obj;
        obj["path"] = result.path();

        FileNameResultAPI resultAPI(const_cast<SearchResult &>(result));
        obj["isDirectory"] = resultAPI.isDirectory();

        if (!resultAPI.isDirectory()) {
            obj["fileType"] = resultAPI.fileType();
            obj["size"] = resultAPI.size();
        }

        QString filename = resultAPI.filename();
        if (!filename.isEmpty()) {
            obj["filename"] = filename;
        }

        QString ext = resultAPI.fileExtension();
        if (!ext.isEmpty()) {
            obj["extension"] = ext;
        }

        obj["isHidden"] = resultAPI.isHidden();

        // 修改时间（包含时间戳和时间字符串）
        qint64 modifyTs = resultAPI.modifyTimestamp();
        if (modifyTs > 0) {
            QJsonObject modifyTimeObj;
            modifyTimeObj["timestamp"] = modifyTs;
            modifyTimeObj["formatted"] = resultAPI.modifyTimeString();
            obj["modifyTime"] = modifyTimeObj;
        }

        // 创建时间（包含时间戳和时间字符串）
        qint64 birthTs = resultAPI.birthTimestamp();
        if (birthTs > 0) {
            QJsonObject birthTimeObj;
            birthTimeObj["timestamp"] = birthTs;
            birthTimeObj["formatted"] = resultAPI.birthTimeString();
            obj["birthTime"] = birthTimeObj;
        }

        return obj;
    } else if (m_searchType == SearchType::Content) {
        // 内容搜索：返回包含路径和内容匹配的对象
        QJsonObject obj;
        obj["path"] = result.path();

        ContentResultAPI resultAPI(const_cast<SearchResult &>(result));
        obj["contentMatch"] = resultAPI.highlightedContent();

        QString filename = resultAPI.filename();
        if (!filename.isEmpty()) {
            obj["filename"] = filename;
        }

        obj["isHidden"] = resultAPI.isHidden();

        // 修改时间
        qint64 modifyTs = resultAPI.modifyTimestamp();
        if (modifyTs > 0) {
            QJsonObject modifyTimeObj;
            modifyTimeObj["timestamp"] = modifyTs;
            modifyTimeObj["formatted"] = resultAPI.modifyTimeString();
            obj["modifyTime"] = modifyTimeObj;
        }

        // 创建时间
        qint64 birthTs = resultAPI.birthTimestamp();
        if (birthTs > 0) {
            QJsonObject birthTimeObj;
            birthTimeObj["timestamp"] = birthTs;
            birthTimeObj["formatted"] = resultAPI.birthTimeString();
            obj["birthTime"] = birthTimeObj;
        }

        return obj;
    } else if (m_searchType == SearchType::Ocr) {
        // OCR 搜索：返回详细对象
        QJsonObject obj;
        obj["path"] = result.path();

        OcrTextResultAPI resultAPI(const_cast<SearchResult &>(result));

        QString ocrContent = resultAPI.ocrContent();
        if (!ocrContent.isEmpty()) {
            obj["ocrContent"] = ocrContent;
        }

        QString filename = resultAPI.filename();
        if (!filename.isEmpty()) {
            obj["filename"] = filename;
        }

        obj["isHidden"] = resultAPI.isHidden();

        // 修改时间
        qint64 modifyTs = resultAPI.modifyTimestamp();
        if (modifyTs > 0) {
            QJsonObject modifyTimeObj;
            modifyTimeObj["timestamp"] = modifyTs;
            modifyTimeObj["formatted"] = resultAPI.modifyTimeString();
            obj["modifyTime"] = modifyTimeObj;
        }

        // 创建时间
        qint64 birthTs = resultAPI.birthTimestamp();
        if (birthTs > 0) {
            QJsonObject birthTimeObj;
            birthTimeObj["timestamp"] = birthTs;
            birthTimeObj["formatted"] = resultAPI.birthTimeString();
            obj["birthTime"] = birthTimeObj;
        }

        return obj;
    }
    return result.path();
}

void JsonOutput::printJsonLine(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    std::cout << doc.toJson(QJsonDocument::Compact).constData() << std::endl;
}

void JsonOutput::outputSearchStarted()
{
    m_startTime = QDateTime::currentDateTime();
    m_collectedResults = QJsonArray();   // 清空收集的结果

    if (m_streaming) {
        outputStreamingStart();
    }
    // 非流式模式：开始时不输出
}

void JsonOutput::outputStreamingStart()
{
    QJsonObject startObj;
    startObj["type"] = "search_started";

    QString searchTypeStr;
    switch (m_searchType) {
    case SearchType::FileName:
        searchTypeStr = "filename";
        break;
    case SearchType::Content:
        searchTypeStr = "content";
        break;
    case SearchType::Ocr:
        searchTypeStr = "ocr";
        break;
    default:
        searchTypeStr = "unknown";
    }

    QJsonObject searchInfo;
    searchInfo["keyword"] = m_keyword;
    searchInfo["searchPath"] = m_searchPath;
    searchInfo["searchType"] = searchTypeStr;
    searchInfo["searchMethod"] = (m_searchMethod == SearchMethod::Indexed ? "indexed" : "realtime");
    searchInfo["caseSensitive"] = m_options.caseSensitive();
    searchInfo["includeHidden"] = m_options.includeHidden();

    // 添加时间范围过滤信息
    if (m_options.hasTimeRangeFilter()) {
        DFMSEARCH::TimeRangeFilter timeFilter = m_options.timeRangeFilter();
        QJsonObject timeFilterInfo;
        timeFilterInfo["field"] = (timeFilter.timeField() == DFMSEARCH::TimeField::BirthTime) ? "birth" : "modify";
        auto [start, end] = timeFilter.resolveTimeRange();
        timeFilterInfo["startTime"] = start.toString(Qt::ISODate);
        timeFilterInfo["endTime"] = end.toString(Qt::ISODate);
        searchInfo["timeRangeFilter"] = timeFilterInfo;
    }

    startObj["search"] = searchInfo;
    startObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    printJsonLine(startObj);
}

void JsonOutput::outputResult(const SearchResult &result)
{
    if (m_streaming) {
        outputStreamingResult(result);
    } else {
        // 非流式模式：收集结果
        m_collectedResults.append(resultToJson(result));
    }
}

void JsonOutput::outputStreamingResult(const SearchResult &result)
{
    QJsonObject resultObj;
    resultObj["type"] = "result";
    resultObj["data"] = resultToJson(result);
    printJsonLine(resultObj);
}

void JsonOutput::outputSearchFinished(const QList<SearchResult> &results)
{
    if (m_streaming) {
        outputStreamingFinish(results);
    } else {
        outputCompleteResult(results);
    }

    emit finished();
}

void JsonOutput::outputStreamingFinish(const QList<SearchResult> &results)
{
    QJsonObject endObj;
    endObj["type"] = "search_finished";

    QJsonObject status;
    status["state"] = "success";
    status["totalResults"] = results.size();

    endObj["status"] = status;

    QJsonObject timestamps;
    timestamps["finished"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    timestamps["duration"] = m_startTime.msecsTo(QDateTime::currentDateTime());

    endObj["timestamps"] = timestamps;
    printJsonLine(endObj);
}

void JsonOutput::outputCompleteResult(const QList<SearchResult> &results)
{
    QJsonObject root;

    // 搜索信息
    QJsonObject searchInfo;
    searchInfo["keyword"] = m_keyword;
    searchInfo["searchPath"] = m_searchPath;

    QString searchTypeStr;
    switch (m_searchType) {
    case SearchType::FileName:
        searchTypeStr = "filename";
        break;
    case SearchType::Content:
        searchTypeStr = "content";
        break;
    case SearchType::Ocr:
        searchTypeStr = "ocr";
        break;
    default:
        searchTypeStr = "unknown";
    }
    searchInfo["searchType"] = searchTypeStr;
    searchInfo["searchMethod"] = (m_searchMethod == SearchMethod::Indexed ? "indexed" : "realtime");
    searchInfo["caseSensitive"] = m_options.caseSensitive();
    searchInfo["includeHidden"] = m_options.includeHidden();

    // 添加时间范围过滤信息
    if (m_options.hasTimeRangeFilter()) {
        DFMSEARCH::TimeRangeFilter timeFilter = m_options.timeRangeFilter();
        QJsonObject timeFilterInfo;
        timeFilterInfo["field"] = (timeFilter.timeField() == DFMSEARCH::TimeField::BirthTime) ? "birth" : "modify";
        auto [start, end] = timeFilter.resolveTimeRange();
        timeFilterInfo["startTime"] = start.toString(Qt::ISODate);
        timeFilterInfo["endTime"] = end.toString(Qt::ISODate);
        searchInfo["timeRangeFilter"] = timeFilterInfo;
    }

    root["search"] = searchInfo;

    // 时间戳
    QDateTime endTime = QDateTime::currentDateTime();
    qint64 duration = m_startTime.msecsTo(endTime);

    QJsonObject timestamps;
    timestamps["started"] = m_startTime.toString(Qt::ISODate);
    timestamps["finished"] = endTime.toString(Qt::ISODate);
    timestamps["duration"] = duration;
    root["timestamps"] = timestamps;

    // 状态
    QJsonObject status;
    status["state"] = "success";
    status["totalResults"] = m_collectedResults.isEmpty() ? results.size() : m_collectedResults.size();
    root["status"] = status;

    // 结果数组
    if (m_collectedResults.isEmpty() && !results.isEmpty()) {
        // 如果结果没有被 resultsFound 收集，则在这里转换
        for (const auto &result : results) {
            m_collectedResults.append(resultToJson(result));
        }
    }
    root["results"] = m_collectedResults;

    // 输出完整 JSON
    QJsonDocument doc(root);
    std::cout << doc.toJson(QJsonDocument::Indented).constData() << std::endl;
}

void JsonOutput::outputSearchCancelled()
{
    QJsonObject cancelObj;
    cancelObj["type"] = "search_cancelled";
    cancelObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    printJsonLine(cancelObj);
    emit finished();
}

void JsonOutput::outputError(const DFMSEARCH::SearchError &error)
{
    QJsonObject errorObj;
    errorObj["type"] = "error";
    errorObj["name"] = error.name();
    errorObj["message"] = error.message();
    errorObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    printJsonLine(errorObj);
}
