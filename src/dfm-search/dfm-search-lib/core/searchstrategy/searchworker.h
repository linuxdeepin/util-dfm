// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CORE_SEARCHWORKER_H
#define CORE_SEARCHWORKER_H

#include <QObject>
#include <memory>
#include <functional>
#include <dfm-search/searchquery.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>
#include "basesearchstrategy.h"

DFM_SEARCH_BEGIN_NS

class SearchStrategyFactory;   // 前向声明

/**
 * @brief 通用搜索工作线程
 */
class SearchWorker : public QObject
{
    Q_OBJECT

public:
    explicit SearchWorker(QObject *parent = nullptr);
    ~SearchWorker();

    void setStrategyFactory(std::unique_ptr<SearchStrategyFactory> factory);

public Q_SLOTS:
    /**
     * @brief 执行搜索操作
     */
    void doSearch(const DFMSEARCH::SearchQuery &query,
                  const DFMSEARCH::SearchOptions &options,
                  DFMSEARCH::SearchType searchType);

    /**
     * @brief 取消搜索操作
     */
    void cancelSearch();

Q_SIGNALS:
    /**
     * @brief 搜索结果信号
     */
    void resultFound(const DFMSEARCH::SearchResult &result);

    /**
     * @brief 搜索完成信号
     */
    void searchFinished(const DFMSEARCH::SearchResultList &results);

    /**
     * @brief 搜索错误信号
     */
    void errorOccurred(const DFMSEARCH::SearchError &error);

private:
    std::unique_ptr<SearchStrategyFactory> m_strategyFactory;
    std::unique_ptr<BaseSearchStrategy> m_strategy;
};

/**
 * @brief 搜索策略工厂接口
 */
class SearchStrategyFactory
{
public:
    virtual ~SearchStrategyFactory() = default;
    virtual std::unique_ptr<BaseSearchStrategy> createStrategy(
            SearchType searchType,
            const SearchOptions &options) = 0;
};

DFM_SEARCH_END_NS

#endif   // CORE_SEARCHWORKER_H
