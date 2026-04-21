// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dfilesorter.h"
#include "dsortkeycache.h"

#include <algorithm>
#include <sys/stat.h>

USING_IO_NAMESPACE

namespace {

// 获取符号链接指向文件的时间信息（用于按时间排序）
struct SymlinkTimeInfo {
    qint64 lastModified = 0;
    qint64 lastModifiedNs = 0;
    qint64 lastRead = 0;
    qint64 lastReadNs = 0;
    bool valid = false;
};

SymlinkTimeInfo getSymlinkTargetTime(const QUrl &symlinkUrl)
{
    SymlinkTimeInfo info;
    if (!symlinkUrl.isValid() || !symlinkUrl.isLocalFile()) {
        return info;
    }

    struct stat st;
    if (stat(symlinkUrl.path().toUtf8().constData(), &st) == 0) {
        info.lastModified = st.st_mtim.tv_sec;
        info.lastModifiedNs = st.st_mtim.tv_nsec;
        info.lastRead = st.st_atim.tv_sec;
        info.lastReadNs = st.st_atim.tv_nsec;
        info.valid = true;
    }
    return info;
}

}   // namespace

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
    // 获取有效大小：目录返回 -1，确保目录始终排在前面
    // 这与原 DLocalHelper::fileSizeByEnt 的行为一致
    auto getEffectiveSize =
            [](const QSharedPointer<DEnumerator::SortFileInfo> &info) -> qint64 {
        // 目录返回 -1
        if (info->isDir) {
            return -1;
        }
        return info->filesize;
    };

    qint64 sizeA = getEffectiveSize(a);
    qint64 sizeB = getEffectiveSize(b);

    // 先按有效大小比较，大小相同时按名称排序
    if (sizeA != sizeB) {
        return sizeA < sizeB;
    }
    return compareByName(a, b);
}

bool DFileSorter::compareByLastModified(
        const QSharedPointer<DEnumerator::SortFileInfo> &a,
        const QSharedPointer<DEnumerator::SortFileInfo> &b) const
{
    // 获取用于排序的修改时间：符号链接使用指向文件的时间
    auto getEffectiveModifiedTime = [](const QSharedPointer<DEnumerator::SortFileInfo> &info)
        -> std::pair<qint64, qint64> {
        // 如果是符号链接且有目标 URL，使用目标文件的时间
        if (info->isSymLink && info->symlinkUrl.isValid()) {
            auto targetTime = getSymlinkTargetTime(info->symlinkUrl);
            if (targetTime.valid) {
                return { targetTime.lastModified, targetTime.lastModifiedNs };
            }
        }
        return { info->lastModifed, info->lastModifedNs };
    };

    auto [timeA, nsA] = getEffectiveModifiedTime(a);
    auto [timeB, nsB] = getEffectiveModifiedTime(b);

    // 先比较秒，再比较纳秒
    if (timeA != timeB) {
        return timeA < timeB;
    }
    if (nsA != nsB) {
        return nsA < nsB;
    }
    // 时间相同，按名称排序
    return compareByName(a, b);
}

bool DFileSorter::compareByLastRead(
        const QSharedPointer<DEnumerator::SortFileInfo> &a,
        const QSharedPointer<DEnumerator::SortFileInfo> &b) const
{
    // 获取用于排序的访问时间：符号链接使用指向文件的时间
    auto getEffectiveReadTime = [](const QSharedPointer<DEnumerator::SortFileInfo> &info)
        -> std::pair<qint64, qint64> {
        // 如果是符号链接且有目标 URL，使用目标文件的时间
        if (info->isSymLink && info->symlinkUrl.isValid()) {
            auto targetTime = getSymlinkTargetTime(info->symlinkUrl);
            if (targetTime.valid) {
                return { targetTime.lastRead, targetTime.lastReadNs };
            }
        }
        return { info->lastRead, info->lastReadNs };
    };

    auto [timeA, nsA] = getEffectiveReadTime(a);
    auto [timeB, nsB] = getEffectiveReadTime(b);

    // 先比较秒，再比较纳秒
    if (timeA != timeB) {
        return timeA < timeB;
    }
    if (nsA != nsB) {
        return nsA < nsB;
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
        if (file->isDir) {
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
