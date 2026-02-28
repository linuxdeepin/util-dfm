// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef FILENAME_SEARCH_ENGINE_H
#define FILENAME_SEARCH_ENGINE_H

#include "core/genericsearchengine.h"

DFM_SEARCH_BEGIN_NS

/**
 * @brief 文件名搜索引擎
 */
class FileNameSearchEngine : public GenericSearchEngine
{
    Q_OBJECT

public:
    explicit FileNameSearchEngine(QObject *parent = nullptr);
    ~FileNameSearchEngine() override;

    // 实现搜索类型
    SearchType searchType() const override { return SearchType::FileName; }

protected:
    // 设置策略工厂
    void setupStrategyFactory() override;

    // 重写验证方法以添加特定验证
    SearchError validateSearchConditions() override;
};

/**
 * @brief 文件名搜索策略工厂
 */
class FileNameSearchStrategyFactory : public SearchStrategyFactory
{
public:
    std::unique_ptr<BaseSearchStrategy> createStrategy(
            SearchType searchType, const SearchOptions &options) override;
};

DFM_SEARCH_END_NS

#endif   // FILENAME_SEARCH_ENGINE_H
