// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RECENTSEARCHSTRATEGY_H
#define RECENTSEARCHSTRATEGY_H

#include "core/searchstrategy/basesearchstrategy.h"

#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/timeresultapi.h>

DFM_SEARCH_BEGIN_NS

struct RecentItem {
    QString path;   // 本地路径 (DBus "Path" 字段)
    QString href;   // file:// URI (DBus "Href" 字段)
    qint64 modified = 0;   // 最近访问时间戳，秒级 Unix epoch
};

/**
 * @brief 搜索策略：从 DBus RecentManager 获取最近使用文件并过滤。
 *
 * 数据流：
 *   1. 调用 org.deepin.Filemanager.Daemon.RecentManager.GetItemsInfo
 *   2. 解析返回的 JSON 数组 → RecentItem 列表
 *   3. 按 SearchQuery 关键词、SearchOptions 中的 fileExtensions + TimeRangeFilter 过滤
 *   4. 发射 resultFound / searchFinished
 *
 * 时间过滤使用 RecentItem.modified 字段（最近访问时间），
 * 而非文件系统的 lastModified——因为"最近使用"的语义是"上次打开时间"。
 */
class RecentSearchStrategy : public BaseSearchStrategy
{
    Q_OBJECT

public:
    explicit RecentSearchStrategy(const SearchOptions &options, QObject *parent = nullptr);
    ~RecentSearchStrategy() override = default;

    SearchType searchType() const override { return SearchType::Recent; }
    void search(const SearchQuery &query) override;
    void cancel() override;

protected:
    // 调用 DBus 获取最近使用记录；失败时返回空列表并记录 warning。
    // 访问权限为 protected 以便测试子类通过 VADDR 宏取到成员函数指针进行打桩。
    QList<RecentItem> fetchRecentItems();

private:

    // ── 过滤管道（每个阶段纯函数，便于测试与组合） ──

    /**
     * @brief 按关键词过滤文件名。
     *
     * 关键词匹配 fileName() 而非完整 path——用户说"打开过的报告"时，
     * "报告"应匹配文件名，不应匹配路径中的目录名。
     * 空关键词时不过滤。
     */
    QList<RecentItem> filterByKeyword(const QList<RecentItem> &items, const QString &keyword) const;

    /**
     * @brief 按扩展名过滤。
     * SearchOptions.fileExtensions 为小写后缀列表（如 ["pdf", "docx"]）。
     * 空列表时不过滤。
     */
    QList<RecentItem> filterByExtensions(const QList<RecentItem> &items, const QStringList &extensions) const;

    /**
     * @brief 按 TimeRangeFilter 过滤 modified 时间戳。
     *
     * 注意：TimeRangeFilter.resolveTimeRange() 返回 QDateTime 对比，
     * 但 RecentItem.modified 是秒级 epoch。需转 QDateTime 比较。
     * start/end 无效时不约束该侧。
     */
    QList<RecentItem> filterByTimeRange(const QList<RecentItem> &items) const;

    /**
     * @brief 将 RecentItem 转换为 SearchResult，填充详细属性。
     */
    SearchResult toSearchResult(const RecentItem &item) const;
};

DFM_SEARCH_END_NS

#endif   // RECENTSEARCHSTRATEGY_H
