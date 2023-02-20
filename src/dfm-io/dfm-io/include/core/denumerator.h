// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    enum class SortRoleCompareFlag : uint8_t {
        kSortRoleCompareDefault = 0,    // The default value
        kSortRoleCompareFileName = 1,   // compare by file path
        kSortRoleCompareFileSize = 2,   // compare by file size
        kSortRoleCompareFileLastModified = 3,   // compare by last modified time
        kSortRoleCompareFileLastRead = 4,   // compare by last read time
    };

    enum class ArgumentKey : uint8_t {
        kArgumentSortRole = 0,    // key of sort role
        kArgumentSortOrder = 1,   // key of sort order
        kArgumentMixDirAndFile = 2,   // key of mix dir and file
    };

    struct SortFileInfo {
        QUrl url;
        bool isFile { false };
        bool isDir { false };
        bool isSymLink { false };
        bool isHide {false };
        bool isReadable {false };
        bool isWriteable {false };
        bool isExecutable {false };
    };

    // call back
    using InitCallbackFunc = void (*)(bool, void *);

    using InitFunc = std::function<bool()>;
    using InitAsyncFunc = std::function<void(int, InitCallbackFunc, void *)>;
    using FileInfoListFunc = std::function<QList<QSharedPointer<DFileInfo>>()>;
    using SetArgumentsFunc = std::function<void(const QMap<DEnumerator::ArgumentKey, QVariant> &)>;
    using SortFileInfoListFunc = std::function<QList<QSharedPointer<DEnumerator::SortFileInfo>>()>;
    using HasNextFunc = std::function<bool()>;
    using NextFunc = std::function<QUrl()>;
    using FileInfoFunc = std::function<QSharedPointer<DFileInfo>()>;
    using FileCountFunc = std::function<quint64()>;
    using CancelFunc = std::function<bool()>;

    using LastErrorFunc = std::function<DFMIOError()>;

public:
    DEnumerator(const QUrl &uri, const QStringList &nameFilters = QStringList(), DirFilters filters = DirFilter::kNoFilter, IteratorFlags flags = IteratorFlag::kNoIteratorFlags);
    virtual ~DEnumerator();

    DFM_VIRTUAL bool init();
    DFM_VIRTUAL void initAsync(int ioPriority = 0, InitCallbackFunc func = nullptr, void *userData = nullptr);
    DFM_VIRTUAL bool cancel();

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
    DFM_VIRTUAL QList<QSharedPointer<DFileInfo>> fileInfoList();
    DFM_VIRTUAL void setArguments(const QMap<DEnumerator::ArgumentKey, QVariant> &argus);
    DFM_VIRTUAL QList<QSharedPointer<DEnumerator::SortFileInfo>> sortFileInfoList();

    DFM_VIRTUAL DFMIOError lastError() const;

    // register
    void registerInit(const InitFunc &func);
    void registerInitAsync(const InitAsyncFunc &func);
    void registerCancel(const CancelFunc &func);
    void registerFileInfoList(const FileInfoListFunc &func);
    void registerSetArguments(const SetArgumentsFunc &func);
    void registerSortFileInfoList(const SortFileInfoListFunc &func);
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

Q_DECLARE_METATYPE(DFMIO::DEnumerator::SortRoleCompareFlag)
Q_DECLARE_METATYPE(QSharedPointer<DFMIO::DEnumerator::SortFileInfo>)
Q_DECLARE_METATYPE(QList<QSharedPointer<DFMIO::DEnumerator::SortFileInfo>>)

#endif   // DENUMERATOR_H
