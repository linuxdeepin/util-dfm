// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LUCENE_COMMIT_LOCK_GUARD_H
#define LUCENE_COMMIT_LOCK_GUARD_H

#include <lucene++/LuceneHeaders.h>
#include <lucene++/FSDirectory.h>
#include <lucene++/Lock.h>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief RAII 守护：在持有期间获取 Lucene 索引的 commit.lock，
 *        防止 IndexWriter commit() 与 IndexReader::open() 并发造成的
 *        段文件不一致读取（表现为 boost::make_shared 内部
 *        "unsorted double linked list corrupted" 崩溃）。
 *
 * 写端 IndexWriter 在 commit() 期间默认会获取 commit.lock；
 * 读端 IndexReader::open(readOnly=true) 不会自动获取任何锁，
 * 故需读端手动 acquire 以同步 commit 窗口。
 *
 * 构造时立即尝试获取锁；析构时若已获取则释放。
 * 失败时 acquired() 返回 false，调用方应放弃本轮读操作而不是阻塞或崩溃。
 */
class LuceneCommitLockGuard
{
public:
    /**
     * @param dir 已带 NativeFSLockFactory 的 FSDirectory
     * @param timeoutMs 每次 attempt 内阻塞等待锁的最长时间
     * @param maxAttempts 总尝试次数；超时后会再次重试直到耗尽
     */
    explicit LuceneCommitLockGuard(const Lucene::FSDirectoryPtr &dir,
                                   int timeoutMs = 1000,
                                   int maxAttempts = 3);

    ~LuceneCommitLockGuard();

    bool acquired() const { return m_acquired; }

    LuceneCommitLockGuard(const LuceneCommitLockGuard &) = delete;
    LuceneCommitLockGuard &operator=(const LuceneCommitLockGuard &) = delete;
    LuceneCommitLockGuard(LuceneCommitLockGuard &&) = delete;
    LuceneCommitLockGuard &operator=(LuceneCommitLockGuard &&) = delete;

private:
    Lucene::LockPtr m_lock;
    bool m_acquired { false };
};

DFM_SEARCH_END_NS

#endif   // LUCENE_COMMIT_LOCK_GUARD_H
