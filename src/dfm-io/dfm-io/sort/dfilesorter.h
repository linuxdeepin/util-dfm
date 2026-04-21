// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILESORTER_H
#define DFILESORTER_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/denumerator.h>

#include <QSharedPointer>
#include <Qt>

#include <functional>

BEGIN_IO_NAMESPACE

/**
 * @brief 文件排序器
 *
 * 提供高性能的文件列表排序功能，支持多种排序角色和排序顺序。
 * 使用 QCollator::sortKey() 进行预缓存排序键，优化性能。
 *
 * 用法示例：
 * @code
 * DFileSorter::SortConfig config;
 * config.role = DFileSorter::SortRole::Name;
 * config.order = Qt::AscendingOrder;
 * config.mixDirAndFile = false;
 *
 * DFileSorter sorter(config);
 * auto sortedList = sorter.sort(std::move(fileList));
 * @endcode
 */
class DFileSorter
{
public:
    /**
     * @brief 排序角色
     */
    enum class SortRole : uint8_t {
        Name = 0,            // 按文件名排序
        Size = 1,            // 按文件大小排序
        LastModified = 2,    // 按最后修改时间排序
        LastRead = 3         // 按最后访问时间排序
    };

    /**
     * @brief 排序配置
     */
    struct SortConfig {
        SortRole role { SortRole::Name };        // 排序角色
        Qt::SortOrder order { Qt::AscendingOrder };   // 排序顺序
        bool mixDirAndFile { false };            // 是否混排目录和文件
    };

    /**
     * @brief 构造函数
     *
     * @param config 排序配置
     */
    explicit DFileSorter(const SortConfig &config);

    /**
     * @brief 对文件列表进行排序
     *
     * @param files 文件列表（使用移动语义避免拷贝）
     * @return 排序后的文件列表
     */
    QList<QSharedPointer<DEnumerator::SortFileInfo>> sort(
        QList<QSharedPointer<DEnumerator::SortFileInfo>> &&files);

    /**
     * @brief 对文件列表进行排序（拷贝版本）
     *
     * @param files 文件列表
     * @return 排序后的文件列表
     */
    QList<QSharedPointer<DEnumerator::SortFileInfo>> sort(
        const QList<QSharedPointer<DEnumerator::SortFileInfo>> &files);

private:
    using CompareFunc = std::function<bool(
        const QSharedPointer<DEnumerator::SortFileInfo> &,
        const QSharedPointer<DEnumerator::SortFileInfo> &)>;

    /**
     * @brief 获取排序比较函数
     */
    CompareFunc getCompareFunc() const;

    /**
     * @brief 按名称比较
     */
    bool compareByName(
        const QSharedPointer<DEnumerator::SortFileInfo> &a,
        const QSharedPointer<DEnumerator::SortFileInfo> &b) const;

    /**
     * @brief 按大小比较
     */
    bool compareBySize(
        const QSharedPointer<DEnumerator::SortFileInfo> &a,
        const QSharedPointer<DEnumerator::SortFileInfo> &b) const;

    /**
     * @brief 按最后修改时间比较
     */
    bool compareByLastModified(
        const QSharedPointer<DEnumerator::SortFileInfo> &a,
        const QSharedPointer<DEnumerator::SortFileInfo> &b) const;

    /**
     * @brief 按最后访问时间比较
     */
    bool compareByLastRead(
        const QSharedPointer<DEnumerator::SortFileInfo> &a,
        const QSharedPointer<DEnumerator::SortFileInfo> &b) const;

    /**
     * @brief 分离目录和文件
     */
    void separateDirAndFile(
        QList<QSharedPointer<DEnumerator::SortFileInfo>> &&files,
        QList<QSharedPointer<DEnumerator::SortFileInfo>> &dirs,
        QList<QSharedPointer<DEnumerator::SortFileInfo>> &regularFiles) const;

    /**
     * @brief 应用排序顺序
     */
    void applySortOrder(QList<QSharedPointer<DEnumerator::SortFileInfo>> &list) const;

private:
    SortConfig m_config;
};

END_IO_NAMESPACE

#endif   // DFILESORTER_H
