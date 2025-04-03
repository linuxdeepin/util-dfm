// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENTSEARCHAPI_H
#define CONTENTSEARCHAPI_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief 内容搜索API
 *
 * 提供内容搜索特有的选项设置
 */
class ContentOptionsAPI
{
public:
    /**
     * @brief 构造函数
     *
     * @param options 要操作的搜索选项对象
     */
    explicit ContentOptionsAPI(SearchOptions &options);

    /**
     * @brief 设置文件类型过滤器
     */
    void setFileTypeFilters(const QStringList &extensions);

    /**
     * @brief 获取文件类型过滤器
     */
    QStringList fileTypeFilters() const;

    /**
     * @brief 设置最大内容预览长度
     */
    void setMaxPreviewLength(int length);

    /**
     * @brief 获取最大内容预览长度
     */
    int maxPreviewLength() const;

    // TODO: html

private:
    SearchOptions &m_options;
};

/**
 * @brief 内容搜索结果API
 *
 */
class ContentResultAPI
{
public:
    ContentResultAPI(SearchResult &result);
    QString highlightedContent() const;
    void setHighlightedContent(const QString &content);

private:
    SearchResult &m_result;
};
DFM_SEARCH_END_NS

#endif   // CONTENTSEARCHAPI_H
