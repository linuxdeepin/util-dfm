// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
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
    std::cout << "Found: " << result.path().toStdString() << std::endl;

    if (m_searchType == SearchType::FileName) {
        FileNameResultAPI resultAPI(const_cast<SearchResult &>(result));

        if (resultAPI.isDirectory()) {
            std::cout << "  Type: Directory" << std::endl;
        } else {
            std::cout << "  Type: " << resultAPI.fileType().toStdString() << std::endl;
            std::cout << "  Size: " << resultAPI.size().toStdString() << " bytes" << std::endl;
        }

        std::cout << "  Modified: " << resultAPI.modifiedTime().toStdString() << std::endl;
    } else if (m_searchType == SearchType::Content) {
        ContentResultAPI contentResult(const_cast<SearchResult &>(result));
        std::cout << "  Content match: " << contentResult.highlightedContent().toStdString() << std::endl;
    } else if (m_searchType == SearchType::Ocr) {
        OcrTextResultAPI ocrResult(const_cast<SearchResult &>(result));
        std::cout << "  OCR text match" << std::endl;
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
