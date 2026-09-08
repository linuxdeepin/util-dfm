// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "lucenecommitlockguard.h"

#include <QDebug>

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

LuceneCommitLockGuard::LuceneCommitLockGuard(const FSDirectoryPtr &dir, int timeoutMs, int maxAttempts)
    : m_lock(dir ? dir->makeLock(L"commit.lock") : nullptr)
    , m_acquired(false)
{
    if (!m_lock) {
        qWarning() << "LuceneCommitLockGuard: failed to create commit.lock handle";
        return;
    }

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        try {
            if (m_lock->obtain(timeoutMs)) {
                m_acquired = true;
                return;
            }
        } catch (const Lucene::LockObtainFailedException &e) {
            qWarning() << "LuceneCommitLockGuard: commit.lock acquire timeout, attempt"
                       << (attempt + 1) << "of" << maxAttempts
                       << ":" << QString::fromStdWString(e.getError());
        } catch (const Lucene::LuceneException &e) {
            qWarning() << "LuceneCommitLockGuard: commit.lock acquire failed, attempt"
                       << (attempt + 1) << "of" << maxAttempts
                       << ":" << QString::fromStdWString(e.getError());
            break;
        }
    }
}

LuceneCommitLockGuard::~LuceneCommitLockGuard()
{
    if (m_acquired && m_lock) {
        try {
            m_lock->release();
        } catch (const Lucene::LuceneException &e) {
            qWarning() << "LuceneCommitLockGuard: failed to release commit.lock:"
                       << QString::fromStdWString(e.getError());
        } catch (const std::exception &e) {
            qWarning() << "LuceneCommitLockGuard: failed to release commit.lock:" << e.what();
        } catch (...) {
            qWarning() << "LuceneCommitLockGuard: unknown exception while releasing commit.lock";
        }
        m_acquired = false;
    }
}

DFM_SEARCH_END_NS
