// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CANCELLABLE_COLLECTOR_H
#define CANCELLABLE_COLLECTOR_H

#include <atomic>
#include <exception>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/Collector.h>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief 搜索取消异常
 * 当搜索被用户取消时抛出此异常
 */
class SearchCancelledException : public std::exception
{
public:
    SearchCancelledException() = default;
    const char *what() const noexcept override { return "Search cancelled by user"; }
};

/**
 * @brief 可取消的文档收集器
 *
 * 继承自 Lucene::Collector，在收集文档过程中检查取消标志
 * 如果检测到取消请求，立即抛出 SearchCancelledException 中断搜索
 */
class CancellableCollector : public Lucene::Collector
{
public:
    /**
     * @brief 构造函数
     * @param cancelled 取消标志的指针（atomic bool）
     * @param maxDocs 最大文档数量限制
     */
    CancellableCollector(std::atomic<bool> *cancelled, int32_t maxDocs);
    ~CancellableCollector() override = default;

    // Lucene::Collector 接口实现
    void setScorer(const Lucene::ScorerPtr &scorer) override;
    void collect(int32_t doc) override;
    void setNextReader(const Lucene::IndexReaderPtr &reader, int32_t docBase) override;
    bool acceptsDocsOutOfOrder() override;

    /**
     * @brief 获取收集到的文档列表
     * @return 按评分排序的文档列表
     */
    Lucene::Collection<Lucene::ScoreDocPtr> getScoreDocs() const;

    /**
     * @brief 获取总命中数量
     * @return 总命中文档数
     */
    int32_t getTotalHits() const { return m_totalHits; }

private:
    std::atomic<bool> *m_cancelled;   // 取消标志指针
    Lucene::Collection<Lucene::ScoreDocPtr> m_scoreDocs;   // 收集的文档
    int32_t m_maxDocs;   // 最大文档数
    int32_t m_docBase;   // 当前段的文档基址
    int32_t m_totalHits;   // 总命中数
    Lucene::ScorerPtr m_scorer;   // 评分器
};

DFM_SEARCH_END_NS

#endif   // CANCELLABLE_COLLECTOR_H
