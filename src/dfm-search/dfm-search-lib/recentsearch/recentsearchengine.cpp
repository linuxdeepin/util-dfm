// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "recentsearchengine.h"
#include "recentstrategies/recentsearchstrategy.h"

#include <dfm-search/searcherror.h>

DFM_SEARCH_BEGIN_NS

RecentSearchEngine::RecentSearchEngine(QObject *parent)
    : GenericSearchEngine(parent)
{
}

RecentSearchEngine::~RecentSearchEngine() = default;

void RecentSearchEngine::setupStrategyFactory()
{
    auto factory = std::make_unique<RecentSearchStrategyFactory>();
    m_worker->setStrategyFactory(std::move(factory));
}

SearchError RecentSearchEngine::validateSearchConditions()
{
    // 最近使用搜索不依赖搜索路径（数据来自 DBus），跳过基类的路径校验。
    // 也不要求关键词非空——"看过的" 单独使用时应返回全部最近文件。
    return SearchError();
}

std::unique_ptr<BaseSearchStrategy> RecentSearchStrategyFactory::createStrategy(
        SearchType searchType, const SearchOptions &options)
{
    if (searchType != SearchType::Recent) {
        return nullptr;
    }
    return std::make_unique<RecentSearchStrategy>(options);
}

DFM_SEARCH_END_NS
