// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dfilesorter.h"
#include "dsortkeycache.h"

#include <algorithm>

USING_IO_NAMESPACE

DFileSorter::DFileSorter(const SortConfig &config)
    : m_config(config)
{
}

QList<QSharedPointer<DEnumerator::SortFileInfo>> DFileSorter::sort(
    QList<QSharedPointer<DEnumerator::SortFileInfo>> &&files)
{
    if (files.isEmpty()) {
        return files;
    }

    QList<QSharedPointer<DEnumerator::SortFileInfo>> result;

    if (m_config.mixDirAndFile) {
        // 混排模式：所有文件一起排序
        std::stable_sort(files.begin(), files.end(), getCompareFunc());
        applySortOrder(files);
        result = std::move(files);
    } else {
        // 分离模式：目录在前，文件在后，分别排序
        QList<QSharedPointer<DEnumerator::SortFileInfo>> dirs;
        QList<QSharedPointer<DEnumerator::SortFileInfo>> regularFiles;

        separateDirAndFile(std::move(files), dirs, regularFiles);

        std::stable_sort(dirs.begin(), dirs.end(), getCompareFunc());
        applySortOrder(dirs);

        std::stable_sort(regularFiles.begin(), regularFiles.end(), getCompareFunc());
        applySortOrder(regularFiles);

        // 目录排在前面
        result = std::move(dirs);
        result.append(std::move(regularFiles));
    }

    return result;
}

QList<QSharedPointer<DEnumerator::SortFileInfo>> DFileSorter::sort(
    const QList<QSharedPointer<DEnumerator::SortFileInfo>> &files)
{
    // 拷贝后排序
    QList<QSharedPointer<DEnumerator::SortFileInfo>> copy = files;
    return sort(std::move(copy));
}

DFileSorter::CompareFunc DFileSorter::getCompareFunc() const
{
    switch (m_config.role) {
    case SortRole::Name:
        return [this](const auto &a, const auto &b) { return compareByName(a, b); };
    case SortRole::Size:
        return [this](const auto &a, const auto &b) { return compareBySize(a, b); };
    case SortRole::LastModified:
        return [this](const auto &a, const auto &b) { return compareByLastModified(a, b); };
    case SortRole::LastRead:
        return [this](const auto &a, const auto &b) { return compareByLastRead(a, b); };
    default:
        return [this](const auto &a, const auto &b) { return compareByName(a, b); };
    }
}

bool DFileSorter::compareByName(
    const QSharedPointer<DEnumerator::SortFileInfo> &a,
    const QSharedPointer<DEnumerator::SortFileInfo> &b) const
{
    // 使用预缓存的排序键进行比较
    QString nameA = a->url.fileName();
    QString nameB = b->url.fileName();

    QCollatorSortKey keyA = DSortKeyCache::instance().sortKey(nameA);
    QCollatorSortKey keyB = DSortKeyCache::instance().sortKey(nameB);

    return keyA < keyB;
}

bool DFileSorter::compareBySize(
    const QSharedPointer<DEnumerator::SortFileInfo> &a,
    const QSharedPointer<DEnumerator::SortFileInfo> &b) const
{
    // 先按大小比较，大小相同时按名称排序
    if (a->filesize != b->filesize) {
        return a->filesize < b->filesize;
    }
    return compareByName(a, b);
}

bool DFileSorter::compareByLastModified(
    const QSharedPointer<DEnumerator::SortFileInfo> &a,
    const QSharedPointer<DEnumerator::SortFileInfo> &b) const
{
    // 先比较秒，再比较纳秒
    if (a->lastModifed != b->lastModifed) {
        return a->lastModifed < b->lastModifed;
    }
    if (a->lastModifedNs != b->lastModifedNs) {
        return a->lastModifedNs < b->lastModifedNs;
    }
    // 时间相同，按名称排序
    return compareByName(a, b);
}

bool DFileSorter::compareByLastRead(
    const QSharedPointer<DEnumerator::SortFileInfo> &a,
    const QSharedPointer<DEnumerator::SortFileInfo> &b) const
{
    // 先比较秒，再比较纳秒
    if (a->lastRead != b->lastRead) {
        return a->lastRead < b->lastRead;
    }
    if (a->lastReadNs != b->lastReadNs) {
        return a->lastReadNs < b->lastReadNs;
    }
    // 时间相同，按名称排序
    return compareByName(a, b);
}

void DFileSorter::separateDirAndFile(
    QList<QSharedPointer<DEnumerator::SortFileInfo>> &&files,
    QList<QSharedPointer<DEnumerator::SortFileInfo>> &dirs,
    QList<QSharedPointer<DEnumerator::SortFileInfo>> &regularFiles) const
{
    dirs.reserve(files.size());
    regularFiles.reserve(files.size());

    for (auto &file : files) {
        if (file->isDir && !file->isSymLink) {
            dirs.append(std::move(file));
        } else {
            regularFiles.append(std::move(file));
        }
    }
}

void DFileSorter::applySortOrder(QList<QSharedPointer<DEnumerator::SortFileInfo>> &list) const
{
    if (m_config.order == Qt::DescendingOrder) {
        std::reverse(list.begin(), list.end());
    }
}
