// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEXT_OUTPUT_H
#define TEXT_OUTPUT_H

#include "output_formatter.h"
#include <dfm-search/searchoptions.h>

namespace dfmsearch {

/**
 * @brief 文本格式输出器
 *
 * 以人类可读的文本格式输出搜索结果
 */
class TextOutput : public OutputFormatter
{
    Q_OBJECT

public:
    explicit TextOutput(QObject *parent = nullptr)
        : OutputFormatter(parent) { }

    void setSearchContext(const QString &keyword, const QString &searchPath,
                          SearchType searchType, SearchMethod searchMethod) override;

    void outputSearchStarted() override;
    void outputResult(const SearchResult &result) override;
    void outputSearchFinished(const QList<SearchResult> &results) override;
    void outputSearchCancelled() override;
    void outputError(const DFMSEARCH::SearchError &error) override;

    /**
     * @brief 设置文件扩展名过滤（用于输出提示）
     */
    void setFileExtensionsFilter(const QString &extensions) { m_fileExtensionsFilter = extensions; }

    /**
     * @brief 设置时间过滤信息
     */
    void setTimeFilterInfo(bool hasFilter, const DFMSEARCH::TimeRangeFilter &filter);

    /**
     * @brief 设置搜索选项
     */
    void setSearchOptions(const SearchOptions &options) { m_options = options; }

private:
    void printSearchResult(const SearchResult &result);

private:
    QString m_keyword;
    QString m_searchPath;
    SearchType m_searchType = SearchType::FileName;
    SearchMethod m_searchMethod = SearchMethod::Indexed;
    SearchOptions m_options;
    QString m_fileExtensionsFilter;
    bool m_hasTimeFilter = false;
    DFMSEARCH::TimeRangeFilter m_timeFilter;
};

}   // namespace dfmsearch

#endif   // TEXT_OUTPUT_H
