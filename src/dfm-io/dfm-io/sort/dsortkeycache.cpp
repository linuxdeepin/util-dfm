// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsortkeycache.h"

USING_IO_NAMESPACE

DSortKeyCache &DSortKeyCache::instance()
{
    static DSortKeyCache instance;
    return instance;
}

QCollatorSortKey DSortKeyCache::sortKey(const QString &text)
{
    // 先尝试读锁查找缓存
    {
        QReadLocker locker(&m_lock);
        auto it = m_cache.find(text);
        if (it != m_cache.end()) {
            return *it;
        }
    }

    // 缓存未命中，需要创建
    QWriteLocker locker(&m_lock);

    // 双重检查，防止其他线程已经创建
    auto it = m_cache.find(text);
    if (it != m_cache.end()) {
        return *it;
    }

    QCollatorSortKey key = collator().sortKey(text);
    m_cache.insert(text, key);
    return key;
}

void DSortKeyCache::clear()
{
    QWriteLocker locker(&m_lock);
    m_cache.clear();
}

QCollator &DSortKeyCache::collator()
{
    // 线程局部存储，每个线程有自己的 QCollator 实例
    // 避免多线程竞争问题
    thread_local static QCollator s_collator = []() {
        QCollator c;
        c.setNumericMode(true);   // 支持数字自然排序，如 "file2" < "file10"
        return c;
    }();
    return s_collator;
}

DSortKeyCache::DSortKeyCache()
{
}
