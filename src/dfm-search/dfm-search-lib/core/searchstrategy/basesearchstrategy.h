// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BASESEARCHSTRATEGY_H
#define BASESEARCHSTRATEGY_H

#include <QObject>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>
#include <dfm-search/searcherror.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief 基础搜索策略接口
 *
 * 为所有类型的搜索提供统一接口
 */
class BaseSearchStrategy : public QObject
{
    Q_OBJECT

public:
    explicit BaseSearchStrategy(const SearchOptions &options, QObject *parent = nullptr)
        : QObject(parent), m_options(options) { }

    virtual ~BaseSearchStrategy() = default;

    /**
     * @brief 设置搜索选项
     */
    virtual void setSearchOptions(const SearchOptions &options) { m_options = options; }

    /**
     * @brief 获取搜索选项
     */
    virtual SearchOptions searchOptions() const { return m_options; }

    /**
     * @brief 获取搜索类型
     */
    virtual SearchType searchType() const = 0;

    /**
     * @brief 执行搜索
     * @param query 搜索查询
     */
    virtual void search(const SearchQuery &query) = 0;

    /**
     * @brief 取消搜索
     */
    virtual void cancel() = 0;

Q_SIGNALS:
    /**
     * @brief 找到搜索结果信号
     */
    void resultFound(const DFMSEARCH::SearchResult &result);

    /**
     * @brief 搜索完成信号
     */
    void searchFinished(const DFMSEARCH::SearchResultList &results);

    /**
     * @brief 搜索出错信号
     */
    void errorOccurred(const DFMSEARCH::SearchError &error);

protected:
    SearchOptions m_options;
    SearchResultList m_results;
    std::atomic<bool> m_cancelled { false };
};

DFM_SEARCH_END_NS

#endif   // BASESEARCHSTRATEGY_H
