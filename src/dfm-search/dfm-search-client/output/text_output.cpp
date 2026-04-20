// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "text_output.h"
#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/ocrtextsearchapi.h>

#include <iostream>

using namespace dfmsearch;
using namespace std;

void TextOutput::setSearchContext(const QString &keyword, const QString &searchPath,
                                  SearchType searchType, SearchMethod searchMethod)
{
    m_keyword = keyword;
    m_searchPath = searchPath;
    m_searchType = searchType;
    m_searchMethod = searchMethod;
}

void TextOutput::setTimeFilterInfo(bool hasFilter, const DFMSEARCH::TimeRangeFilter &filter)
{
    m_hasTimeFilter = hasFilter;
    m_timeFilter = filter;
}

void TextOutput::outputSearchStarted()
{
    std::cout << "Search started..." << std::endl;
    std::cout << "Searching for: " << m_keyword.toStdString() << std::endl;
    std::cout << "In path: " << m_searchPath.toStdString() << std::endl;

    QString typeStr = "Filename";
    if (m_searchType == SearchType::Content)
        typeStr = "Content";
    else if (m_searchType == SearchType::Ocr)
        typeStr = "Ocr";
    std::cout << "Search type: " << typeStr.toStdString() << std::endl;
    std::cout << "Search method: " << (m_searchMethod == SearchMethod::Indexed ? "Indexed" : "Realtime") << std::endl;

    // 打印文件扩展名过滤
    if (!m_fileExtensionsFilter.isEmpty()) {
        std::cout << "File extensions filter: " << m_fileExtensionsFilter.toStdString() << std::endl;
    }

    // 打印时间范围过滤
    if (m_hasTimeFilter) {
        std::cout << "Time range filter: ";
        std::cout << (m_timeFilter.timeField() == DFMSEARCH::TimeField::BirthTime ? "birth_time" : "modify_time");
        auto [start, end] = m_timeFilter.resolveTimeRange();
        std::cout << " from " << start.toString("yyyy-MM-dd HH:mm:ss").toStdString()
                  << " to " << end.toString("yyyy-MM-dd HH:mm:ss").toStdString();
        std::cout << std::endl;
    }
}

void TextOutput::printSearchResult(const SearchResult &result)
{
    // 非详细模式下只输出路径
    if (!m_verbose) {
        std::cout << result.path().toStdString() << std::endl;
        return;
    }

    // 详细模式下输出所有信息
    std::cout << "Found: " << result.path().toStdString() << std::endl;

    if (m_searchType == SearchType::FileName) {
        FileNameResultAPI resultAPI(const_cast<SearchResult &>(result));

        if (resultAPI.isDirectory()) {
            std::cout << "  Type: Directory" << std::endl;
        } else {
            std::cout << "  Type: " << resultAPI.fileType().toStdString() << std::endl;
            std::cout << "  Size: " << resultAPI.size().toStdString() << " bytes" << std::endl;
        }

        // 文件名和扩展名
        QString filename = resultAPI.filename();
        if (!filename.isEmpty()) {
            std::cout << "  Filename: " << filename.toStdString() << std::endl;
        }
        QString ext = resultAPI.fileExtension();
        if (!ext.isEmpty()) {
            std::cout << "  Extension: " << ext.toStdString() << std::endl;
        }

        // 隐藏状态
        std::cout << "  Hidden: " << (resultAPI.isHidden() ? "Yes" : "No") << std::endl;

        // 修改时间（同时输出时间戳和时间字符串）
        qint64 modifyTs = resultAPI.modifyTimestamp();
        if (modifyTs > 0) {
            std::cout << "  Modified: " << resultAPI.modifyTimeString().toStdString()
                      << " (timestamp: " << modifyTs << ")" << std::endl;
        }

        // 创建时间（同时输出时间戳和时间字符串）
        qint64 birthTs = resultAPI.birthTimestamp();
        if (birthTs > 0) {
            std::cout << "  Created: " << resultAPI.birthTimeString().toStdString()
                      << " (timestamp: " << birthTs << ")" << std::endl;
        }
    } else if (m_searchType == SearchType::Content) {
        ContentResultAPI resultAPI(const_cast<SearchResult &>(result));

        std::cout << "  Content match: " << resultAPI.highlightedContent().toStdString() << std::endl;

        // 文件名
        QString filename = resultAPI.filename();
        if (!filename.isEmpty()) {
            std::cout << "  Filename: " << filename.toStdString() << std::endl;
        }

        // 隐藏状态
        std::cout << "  Hidden: " << (resultAPI.isHidden() ? "Yes" : "No") << std::endl;

        // 修改时间
        qint64 modifyTs = resultAPI.modifyTimestamp();
        if (modifyTs > 0) {
            std::cout << "  Modified: " << resultAPI.modifyTimeString().toStdString()
                      << " (timestamp: " << modifyTs << ")" << std::endl;
        }

        // 创建时间
        qint64 birthTs = resultAPI.birthTimestamp();
        if (birthTs > 0) {
            std::cout << "  Created: " << resultAPI.birthTimeString().toStdString()
                      << " (timestamp: " << birthTs << ")" << std::endl;
        }
    } else if (m_searchType == SearchType::Ocr) {
        OcrTextResultAPI resultAPI(const_cast<SearchResult &>(result));

        QString ocrContent = resultAPI.ocrContent();
        if (!ocrContent.isEmpty()) {
            std::cout << "  OCR content: " << ocrContent.toStdString() << std::endl;
        }

        // 文件名
        QString filename = resultAPI.filename();
        if (!filename.isEmpty()) {
            std::cout << "  Filename: " << filename.toStdString() << std::endl;
        }

        // 隐藏状态
        std::cout << "  Hidden: " << (resultAPI.isHidden() ? "Yes" : "No") << std::endl;

        // 修改时间
        qint64 modifyTs = resultAPI.modifyTimestamp();
        if (modifyTs > 0) {
            std::cout << "  Modified: " << resultAPI.modifyTimeString().toStdString()
                      << " (timestamp: " << modifyTs << ")" << std::endl;
        }

        // 创建时间
        qint64 birthTs = resultAPI.birthTimestamp();
        if (birthTs > 0) {
            std::cout << "  Created: " << resultAPI.birthTimeString().toStdString()
                      << " (timestamp: " << birthTs << ")" << std::endl;
        }
    }

    std::cout << std::endl;
}

void TextOutput::outputResult(const SearchResult &result)
{
    printSearchResult(result);
}

void TextOutput::outputSearchFinished(const QList<SearchResult> &results)
{
    std::cout << "Search finished. Total results: " << results.size() << std::endl;

    // 如果禁用了 resultFoundEnabled，则在这里输出结果
    if (!m_options.resultFoundEnabled()) {
        for (const auto &result : results) {
            printSearchResult(result);
        }
    }

    emit finished();
}

void TextOutput::outputSearchCancelled()
{
    std::cout << "Search cancelled" << std::endl;
    emit finished();
}

void TextOutput::outputError(const DFMSEARCH::SearchError &error)
{
    std::cerr << "[Error]: " << error.code()
              << "[Name]: " << error.name().toStdString()
              << "[Message]:" << error.message().toStdString() << std::endl;
}
