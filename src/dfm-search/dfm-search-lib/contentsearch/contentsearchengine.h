// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENT_SEARCH_ENGINE_H
#define CONTENT_SEARCH_ENGINE_H

#include "core/genericsearchengine.h"

DFM_SEARCH_BEGIN_NS

/**
 * @brief 内容搜索引擎
 *
 * 实现基于文件内容的搜索功能
 */
class ContentSearchEngine : public GenericSearchEngine
{
    Q_OBJECT

public:
    explicit ContentSearchEngine(QObject *parent = nullptr);
    ~ContentSearchEngine() override;

    // 实现搜索类型
    SearchType searchType() const override { return SearchType::Content; }

protected:
    // 设置策略工厂
    void setupStrategyFactory() override;

    // 重写验证方法以添加特定验证
    SearchError validateSearchConditions() override;
};

/**
 * @brief 内容搜索策略工厂
 */
class ContentSearchStrategyFactory : public SearchStrategyFactory
{
public:
    std::unique_ptr<BaseSearchStrategy> createStrategy(
            SearchType searchType, const SearchOptions &options) override;
};

DFM_SEARCH_END_NS

#endif   // CONTENT_SEARCH_ENGINE_H
