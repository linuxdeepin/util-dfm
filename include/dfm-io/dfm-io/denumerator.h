// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DENUMERATOR_H
#define DENUMERATOR_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/error/error.h>

#include <QUrl>
#include <QSharedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DEnumeratorPrivate;
class DFileInfo;
class DEnumeratorFuture;
class DEnumerator : public QEnableSharedFromThis<DEnumerator>
{
public:
    enum class DirFilter : int16_t {
        kNoFilter = -1,   // no filter
        kDirs = 0x001,   // List directories that match the filters.
        kFiles = 0x002,   // List files.
        kDrives = 0x004,   // List disk drives (ignored under Unix).
        kAllEntries = 0x001 | 0x002 | 0x004,   // List directories, files, drives and symlinks (this does not list broken symlinks unless you specify System).
        kNoSymLinks = 0x008,   // Do not list symbolic links (ignored by operating systems that don't support symbolic links).

        kReadable = 0x010,   // List files for which the application has read access. The Readable value needs to be combined with Dirs or Files.
        kWritable = 0x020,   // List files for which the application has write access. The Writable value needs to be combined with Dirs or Files.
        kExecutable = 0x040,   // List files for which the application has execute access. The Executable value needs to be combined with Dirs or Files.
        kModified = 0x080,   // Only list files that have been modified (ignored on Unix).

        kHidden = 0x100,   // List hidden files (on Unix, files starting with a ".").
        kSystem = 0x200,   // List system files (on Unix, FIFOs, sockets and device files are included; on Windows, .lnk files are included)
        kAllDirs = 0x400,   // List all directories; i.e. don't apply the filters to directory names.
        kCaseSensitive = 0x800,   // The filter should be case sensitive.

        kNoDot = 0x2000,   // Do not list the special entry ".".
        kNoDotDot = 0x4000,   // Do not list the special entry "..".
        kNoDotAndDotDot = 0x2000 | 0x4000,   // Do not list the special entries "." and "..".
    };
    Q_DECLARE_FLAGS(DirFilters, DirFilter)

    enum class IteratorFlag : uint8_t {
        kNoIteratorFlags = 0x00,   // The default value, representing no flags. The iterator will return entries for the assigned path.
        kFollowSymlinks = 0x01,   // When combined with Subdirectories, this flag enables iterating through all subdirectories of the assigned path, following all symbolic links. Symbolic link loops (e.g., "link" => "." or "link" => "..") are automatically detected and ignored.
        kSubdirectories = 0x02,   // List entries inside all subdirectories as well.
    };
    Q_DECLARE_FLAGS(IteratorFlags, IteratorFlag)

    enum class SortRoleCompareFlag : uint8_t {
        kSortRoleCompareDefault = 0,   // The default value
        kSortRoleCompareFileName = 1,   // compare by file path
        kSortRoleCompareFileSize = 2,   // compare by file size
        kSortRoleCompareFileLastModified = 3,   // compare by last modified time
        kSortRoleCompareFileLastRead = 4,   // compare by last read time
    };

    struct SortFileInfo
    {
        QUrl url;
        qint64 filesize { 0 };
        bool isFile { false };
        bool isDir { false };
        bool isSymLink { false };
        bool isHide { false };
        bool isReadable { false };
        bool isWriteable { false };
        bool isExecutable { false };
        ino_t inode { 0 };
        QUrl symlinkUrl;
        uint gid { 0 };
        uint uid { 0 };
        qint64 lastRead { 0 };
        qint64 lastReadNs { 0 };
        qint64 lastModifed { 0 };
        qint64 lastModifedNs { 0 };
        qint64 create { 0 };
        qint64 createNs { 0 };
    };

    enum class EnumeratorType : uint8_t{
        kEnumeratorGio = 0, // enumerator by gio
        kEnumeratorFts = 0, // enumerator by fts
        kEnumeratorSystem = 0, // enumerator by system dirent
    };

public:
    explicit DEnumerator(const QUrl &uri);
    explicit DEnumerator(const QUrl &uri, const QStringList &nameFilters, DirFilters filters, IteratorFlags flags);
    ~DEnumerator();
    QUrl uri() const;

    void setNameFilters(const QStringList &filters);
    QStringList nameFilters() const;

    void setDirFilters(DirFilters filters);
    DirFilters dirFilters() const;

    void setIteratorFlags(IteratorFlags flags);
    IteratorFlags iteratorFlags() const;

    void setTimeout(ulong timeout);
    ulong timeout() const;

    void setSortRole(SortRoleCompareFlag role);
    SortRoleCompareFlag sortRole() const;

    void setSortOrder(Qt::SortOrder order);
    Qt::SortOrder sortOrder() const;

    void setSortMixed(bool mix);
    bool isSortMixed() const;

    void setQueryAttributes(const QString &attributes);
    QString queryAttributes() const;

public:
    bool cancel();
    bool hasNext() const;
    QUrl next() const;
    QSharedPointer<DFileInfo> fileInfo() const;
    quint64 fileCount();
    QList<QSharedPointer<DFileInfo>> fileInfoList();
    QList<QSharedPointer<DEnumerator::SortFileInfo>> sortFileInfoList();
    DFMIOError lastError() const;
    DEnumeratorFuture *asyncIterator();
    void startAsyncIterator();
    bool isAsyncOver() const;
    bool initEnumerator(const bool oneByone = true);

private:
    QSharedPointer<DEnumeratorPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DEnumerator::DirFilters);
Q_DECLARE_OPERATORS_FOR_FLAGS(DEnumerator::IteratorFlags);

END_IO_NAMESPACE

Q_DECLARE_METATYPE(DFMIO::DEnumerator::SortRoleCompareFlag)
Q_DECLARE_METATYPE(QSharedPointer<DFMIO::DEnumerator::SortFileInfo>)
Q_DECLARE_METATYPE(QList<QSharedPointer<DFMIO::DEnumerator::SortFileInfo>>)

#endif   // DENUMERATOR_H
