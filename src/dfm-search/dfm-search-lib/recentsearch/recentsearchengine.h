// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RECENTSEARCHENGINE_H
#define RECENTSEARCHENGINE_H

#include "core/genericsearchengine.h"

DFM_SEARCH_BEGIN_NS

/**
 * @brief 搜索引擎：最近使用文件（DBus RecentManager 数据源）
 *
 * 数据源为 org.deepin.Filemanager.Daemon.RecentManager.GetItemsInfo，
 * 返回 JSON 数组，每条含 Href/Path/modified。
 *
 * 该引擎不复用 lucene++ 索引——最近使用记录不在索引中，
 * 因此所有 filename/content 搜索路径都无法触及此数据。
 */
class RecentSearchEngine : public GenericSearchEngine
{
    Q_OBJECT

public:
    explicit RecentSearchEngine(QObject *parent = nullptr);
    ~RecentSearchEngine() override;

    SearchType searchType() const override { return SearchType::Recent; }

protected:
    void setupStrategyFactory() override;
    SearchError validateSearchConditions() override;
};

/**
 * @brief 策略工厂：始终创建 RecentSearchStrategy（无 indexed/realtime 分支）
 */
class RecentSearchStrategyFactory : public SearchStrategyFactory
{
public:
    std::unique_ptr<BaseSearchStrategy> createStrategy(
            SearchType searchType, const SearchOptions &options) override;
};

DFM_SEARCH_END_NS

#endif   // RECENTSEARCHENGINE_H
