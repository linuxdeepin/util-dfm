// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef FILENAME_BASE_STRATEGY_H
#define FILENAME_BASE_STRATEGY_H

#include "core/searchstrategy/basesearchstrategy.h"
#include <dfm-search/filenamesearchapi.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief 文件名搜索策略基类
 */
class FileNameBaseStrategy : public BaseSearchStrategy
{
    Q_OBJECT

public:
    explicit FileNameBaseStrategy(const SearchOptions &options, QObject *parent = nullptr)
        : BaseSearchStrategy(options, parent) { }

    SearchType searchType() const override { return SearchType::FileName; }
};

DFM_SEARCH_END_NS

#endif   // FILENAME_BASE_STRATEGY_H
