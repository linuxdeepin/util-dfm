// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "searchworker.h"

DFM_SEARCH_BEGIN_NS

SearchWorker::SearchWorker(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<SearchQuery>();
    qRegisterMetaType<SearchOptions>();
    qRegisterMetaType<SearchType>();
    qRegisterMetaType<SearchResultList>();
    qRegisterMetaType<SearchError>();
}

SearchWorker::~SearchWorker() = default;

void SearchWorker::setStrategyFactory(std::unique_ptr<SearchStrategyFactory> factory)
{
    m_strategyFactory = std::move(factory);
}

void SearchWorker::doSearch(const SearchQuery &query,
                            const SearchOptions &options,
                            SearchType searchType)
{
    if (!m_strategyFactory) {
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
        return;
    }

    // 断开之前的连接
    if (m_strategy) {
        disconnect(m_strategy.get(), nullptr, this, nullptr);
    }

    // 创建策略
    m_strategy = m_strategyFactory->createStrategy(searchType, options);
    if (!m_strategy) {
        emit errorOccurred(SearchError(SearchErrorCode::InternalError));
        return;
    }

    // 连接信号
    connect(m_strategy.get(), &BaseSearchStrategy::resultFound,
            this, &SearchWorker::resultFound);
    connect(m_strategy.get(), &BaseSearchStrategy::searchFinished,
            this, &SearchWorker::searchFinished);
    connect(m_strategy.get(), &BaseSearchStrategy::errorOccurred,
            this, &SearchWorker::errorOccurred);

    // 执行搜索
    m_strategy->search(query);
}

void SearchWorker::cancelSearch()
{
    if (m_strategy) {
        m_strategy->cancel();
    }
}

DFM_SEARCH_END_NS
