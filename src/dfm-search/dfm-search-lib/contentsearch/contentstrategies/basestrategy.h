// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENT_BASE_STRATEGY_H
#define CONTENT_BASE_STRATEGY_H

#include "core/searchstrategy/basesearchstrategy.h"
#include <dfm-search/contentsearchapi.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief 内容搜索策略基类
 */
class ContentBaseStrategy : public BaseSearchStrategy
{
    Q_OBJECT

public:
    explicit ContentBaseStrategy(const SearchOptions &options, QObject *parent = nullptr)
        : BaseSearchStrategy(options, parent) { }

    SearchType searchType() const override { return SearchType::Content; }

protected:
    // 公共工具方法
    int getMaxPreviewLength();
};

DFM_SEARCH_END_NS

#endif   // CONTENT_BASE_STRATEGY_H
