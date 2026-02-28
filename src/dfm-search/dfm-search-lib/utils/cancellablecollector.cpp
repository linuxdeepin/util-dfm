// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "cancellablecollector.h"

#include <QDebug>

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

CancellableCollector::CancellableCollector(std::atomic<bool> *cancelled, int32_t maxDocs)
    : m_cancelled(cancelled), m_maxDocs(maxDocs), m_docBase(0), m_totalHits(0)
{
    // 预分配空间以提高性能
    m_scoreDocs = Collection<ScoreDocPtr>::newInstance();
}

void CancellableCollector::setScorer(const ScorerPtr &scorer)
{
    if (m_cancelled && m_cancelled->load()) {
        // 抛出异常中断搜索过程
        throw SearchCancelledException();
    }

    m_scorer = scorer;
}

void CancellableCollector::collect(int32_t doc)
{
    // 关键：每次收集文档时检查取消标志
    // 这是实现搜索可中断的核心机制
    if (m_cancelled && m_cancelled->load()) {
        // 抛出异常中断搜索过程
        throw SearchCancelledException();
    }

    m_totalHits++;

    // 只收集不超过最大数量的文档
    if (m_scoreDocs.size() < m_maxDocs) {
        try {
            // 获取当前文档的评分
            double score = m_scorer ? m_scorer->score() : 0.0;

            // 创建评分文档对象
            ScoreDocPtr scoreDoc = newLucene<ScoreDoc>(m_docBase + doc, score);

            // 添加到结果集合
            m_scoreDocs.add(scoreDoc);
        } catch (const LuceneException &e) {
            qWarning() << "Error collecting document:" << QString::fromStdWString(e.getError());
        } catch (const std::exception &e) {
            qWarning() << "Standard exception in collect:" << e.what();
        }
    }
}

void CancellableCollector::setNextReader(const IndexReaderPtr &reader, int32_t docBase)
{
    Q_UNUSED(reader);
    if (m_cancelled && m_cancelled->load()) {
        // 抛出异常中断搜索过程
        throw SearchCancelledException();
    }

    // 设置当前段的文档基址，用于计算全局文档 ID
    m_docBase = docBase;
}

bool CancellableCollector::acceptsDocsOutOfOrder()
{
    // 返回 true 表示可以接受乱序的文档，这可以提高性能
    // 因为我们不依赖文档的顺序
    return true;
}

Collection<ScoreDocPtr> CancellableCollector::getScoreDocs() const
{
    return m_scoreDocs;
}

DFM_SEARCH_END_NS
