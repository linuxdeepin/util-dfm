// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "contentsearchengine.h"

#include "contentstrategies/indexedstrategy.h"

DFM_SEARCH_BEGIN_NS
DCORE_USE_NAMESPACE

ContentSearchEngine::ContentSearchEngine(QObject *parent)
    : GenericSearchEngine(parent)
{
}

ContentSearchEngine::~ContentSearchEngine() = default;

void ContentSearchEngine::setupStrategyFactory()
{
    // 设置内容搜索策略工厂
    auto factory = std::make_unique<ContentSearchStrategyFactory>();
    m_worker->setStrategyFactory(std::move(factory));
}

SearchError ContentSearchEngine::validateSearchConditions()
{
    // 先执行基类验证
    auto result = GenericSearchEngine::validateSearchConditions();
    if (result.isError()) {
        return result;
    }

    // 内容搜索特定验证
    ContentOptionsAPI api(m_options);
    if (m_options.method() != SearchMethod::Indexed) {
        return SearchError(SearchErrorCode::InvalidSerchMethod);
    }

    // 检查是否为不支持的 Wildcard 查询类型
    if (m_currentQuery.type() == SearchQuery::Type::Wildcard) {
        return SearchError(ContentSearchErrorCode::WildcardNotSupported);
    }

    if (m_currentQuery.type() == SearchQuery::Type::Simple
        && m_currentQuery.keyword().toUtf8().size() < Global::kMinContentSearchKeywordLength) {
        return SearchError(ContentSearchErrorCode::KeywordTooShort);
    }

    return result;
}

std::unique_ptr<BaseSearchStrategy> ContentSearchStrategyFactory::createStrategy(
        SearchType searchType, const SearchOptions &options)
{
    // 确保搜索类型正确
    if (searchType != SearchType::Content) {
        return nullptr;
    }

    // 根据搜索方法创建对应的策略
    if (options.method() == SearchMethod::Indexed) {
        return std::make_unique<ContentIndexedStrategy>(options);
    }

    return nullptr;
}

DFM_SEARCH_END_NS
