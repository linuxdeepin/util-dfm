// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSORTKEYCACHE_H
#define DSORTKEYCACHE_H

#include <dfm-io/dfmio_global.h>

#include <QCollator>
#include <QCollatorSortKey>
#include <QHash>
#include <QReadWriteLock>
#include <QString>

BEGIN_IO_NAMESPACE

/**
 * @brief 排序键缓存管理器
 *
 * 单例模式，提供线程安全的 QCollatorSortKey 缓存。
 * 使用 QCollator 配置 numericMode 进行国际化自然排序。
 *
 * 性能优化：
 * - 同一字符串只生成一次排序键
 * - 线程安全的 QCollator 实例（thread_local）
 * - 读写锁保护缓存访问
 */
class DSortKeyCache
{
public:
    /**
     * @brief 获取单例实例
     */
    static DSortKeyCache &instance();

    /**
     * @brief 获取或创建字符串的排序键
     *
     * 如果缓存中存在则直接返回，否则创建新排序键并缓存。
     *
     * @param text 需要排序的文本
     * @return QCollatorSortKey 排序键
     */
    QCollatorSortKey sortKey(const QString &text);

    /**
     * @brief 清理缓存
     *
     * 在语言环境变化时调用。
     */
    void clear();

    /**
     * @brief 获取配置好的 QCollator 实例
     *
     * 线程安全，每个线程有自己的实例。
     *
     * @return QCollator& QCollator 引用
     */
    static QCollator &collator();

private:
    DSortKeyCache();
    ~DSortKeyCache() = default;
    DSortKeyCache(const DSortKeyCache &) = delete;
    DSortKeyCache &operator=(const DSortKeyCache &) = delete;

    QHash<QString, QCollatorSortKey> m_cache;
    QReadWriteLock m_lock;
};

END_IO_NAMESPACE

#endif   // DSORTKEYCACHE_H
