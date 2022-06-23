/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DENUMERATOR_H
#define DENUMERATOR_H

#include "dfmio_global.h"

#include "error/error.h"

#include <QSharedPointer>
#include <QUrl>

#include <functional>

const int16_t kDirFilterNofilter = -1;
const uint16_t kDirFilterDirs = 0x001;
const uint16_t kDirFilterFiles = 0x002;
const uint16_t kDirFilterDrives = 0x004;
const uint16_t kDirFilterAllEntries = 0x001 | 0x002 | 0x004;
const uint16_t kDirFilterNoSymLinks = 0x008;

const uint16_t kDirFilterReadable = 0x010;
const uint16_t kDirFilterWritable = 0x020;
const uint16_t kDirFilterExecutable = 0x040;
const uint16_t kDirFilterModified = 0x080;

const uint16_t kDirFilterHidden = 0x100;
const uint16_t kDirFilterSystem = 0x200;
const uint16_t kDirFilterAllDirs = 0x400;
const uint16_t kDirFilterCaseSensitive = 0x800;

const uint16_t kDirFilterNoDot = 0x2000;
const uint16_t kDirFilterNoDotDot = 0x4000;
const uint16_t kDirFilterNoDotAndDotDot = 0x2000 | 0x4000;

BEGIN_IO_NAMESPACE

class DEnumeratorPrivate;
class DFileInfo;

class DEnumerator
{
public:
    enum class DirFilter : int16_t {
        kNoFilter = kDirFilterNofilter,   // no filter
        kDirs = kDirFilterDirs,   // List directories that match the filters.
        kFiles = kDirFilterFiles,   // List files.
        kDrives = kDirFilterDrives,   // List disk drives (ignored under Unix).
        kAllEntries = kDirFilterAllEntries,   // List directories, files, drives and symlinks (this does not list broken symlinks unless you specify System).
        kNoSymLinks = kDirFilterNoSymLinks,   // Do not list symbolic links (ignored by operating systems that don't support symbolic links).

        kReadable = kDirFilterReadable,   // List files for which the application has read access. The Readable value needs to be combined with Dirs or Files.
        kWritable = kDirFilterWritable,   // List files for which the application has write access. The Writable value needs to be combined with Dirs or Files.
        kExecutable = kDirFilterExecutable,   // List files for which the application has execute access. The Executable value needs to be combined with Dirs or Files.
        kModified = kDirFilterModified,   // Only list files that have been modified (ignored on Unix).

        kHidden = kDirFilterHidden,   // List hidden files (on Unix, files starting with a ".").
        kSystem = kDirFilterSystem,   // List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included)
        kAllDirs = kDirFilterAllDirs,   // List all directories; i.e. don't apply the filters to directory names.
        kCaseSensitive = kDirFilterCaseSensitive,   // The filter should be case sensitive.

        kNoDot = kDirFilterNoDot,   // Do not list the special entry ".".
        kNoDotDot = kDirFilterNoDotDot,   // Do not list the special entry "..".
        kNoDotAndDotDot = kDirFilterNoDotAndDotDot,   // Do not list the special entries "." and "..".
    };
    Q_DECLARE_FLAGS(DirFilters, DirFilter)

    enum class IteratorFlag : uint8_t {
        kNoIteratorFlags = 0x00,   // The default value, representing no flags. The iterator will return entries for the assigned path.
        kFollowSymlinks = 0x01,   // When combined with Subdirectories, this flag enables iterating through all subdirectories of the assigned path, following all symbolic links. Symbolic link loops (e.g., "link" => "." or "link" => "..") are automatically detected and ignored.
        kSubdirectories = 0x02,   // List entries inside all subdirectories as well.
    };
    Q_DECLARE_FLAGS(IteratorFlags, IteratorFlag)

    using FileInfoListFunc = std::function<QList<QSharedPointer<DFileInfo>>()>;
    using HasNextFunc = std::function<bool()>;
    using NextFunc = std::function<QUrl()>;
    using FileInfoFunc = std::function<QSharedPointer<DFileInfo>()>;
    using FileCountFunc = std::function<quint64()>;

    using LastErrorFunc = std::function<DFMIOError()>;

public:
    DEnumerator(const QUrl &uri, const QStringList &nameFilters = QStringList(), DirFilters filters = DirFilter::kNoFilter, IteratorFlags flags = IteratorFlag::kNoIteratorFlags);

    virtual ~DEnumerator();

    QUrl uri() const;
    QStringList nameFilters() const;
    DirFilters dirFilters();
    IteratorFlags iteratorFlags() const;
    void setTimeout(ulong timeout);
    ulong timeout() const;

    DFM_VIRTUAL bool hasNext() const;
    DFM_VIRTUAL QUrl next() const;
    DFM_VIRTUAL QSharedPointer<DFileInfo> fileInfo() const;
    DFM_VIRTUAL quint64 fileCount();

    DFM_VIRTUAL DFMIOError lastError() const;

    // register
    void registerFileInfoList(const FileInfoListFunc &func);
    void registerHasNext(const HasNextFunc &func);
    void registerNext(const NextFunc &func);
    void registerFileInfo(const FileInfoFunc &func);
    void registerFileCount(const FileCountFunc &func);
    void registerLastError(const LastErrorFunc &func);

private:
    QSharedPointer<DEnumeratorPrivate> d = nullptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DEnumerator::DirFilters);
Q_DECLARE_OPERATORS_FOR_FLAGS(DEnumerator::IteratorFlags);

END_IO_NAMESPACE

#endif   // DENUMERATOR_H
