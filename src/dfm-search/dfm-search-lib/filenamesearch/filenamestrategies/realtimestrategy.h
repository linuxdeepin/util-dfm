// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef FILENAME_REALTIME_STRATEGY_H
#define FILENAME_REALTIME_STRATEGY_H

#include "basestrategy.h"

DFM_SEARCH_BEGIN_NS

/**
 * @brief 文件名实时搜索策略
 */
class FileNameRealTimeStrategy : public FileNameBaseStrategy
{
    Q_OBJECT

public:
    explicit FileNameRealTimeStrategy(const SearchOptions &options, QObject *parent = nullptr);
    ~FileNameRealTimeStrategy() override;

    void search(const SearchQuery &query) override;
    void cancel() override;

private:
    // 拼音匹配
    bool matchPinyin(const QString &fileName, const QString &keyword);

    // 布尔查询匹配
    bool matchBoolean(const QString &fileName, const SearchQuery &query,
                      bool caseSensitive, bool pinyinEnabled);

    // 通配符匹配
    bool matchWildcard(const QString &fileName, const QString &pattern, bool caseSensitive);
};

DFM_SEARCH_END_NS

#endif   // FILENAME_REALTIME_STRATEGY_H
