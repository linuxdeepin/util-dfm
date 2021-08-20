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
#ifndef DFILEINFO_H
#define DFILEINFO_H

#include "dfmio_global.h"

#include "error/error.h"

#include <QUrl>
#include <QSharedData>
#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DFileInfoPrivate;

class DFileInfo
{
public:
    enum class DFileType : uint16_t {
        Unknown = 0,                   // File's type is unknown.
        Regular = 1,                   // File handle represents a regular file.
        Directory = 2,                 // File handle represents a directory.
        SymbolicLink = 3,              // File handle represents a symbolic link (Unix systems).
        Special = 4,                   // File is a "special" file, such as a socket, fifo, block device, or character device.
        Shortcut = 5,                  // File is a shortcut (Windows systems).
        Mountable = 6,                 // File is a mountable location.

        // Reserved
        UserType = 0x100
    };

    enum class AttributeID : uint16_t {
        StandardType = 0,              // DFileType
        StandardIsHiden = 1,
        StandardIsBackup = 2,
        StandardIsSymlink = 3,
        standardIsVirtual = 4,
        StandardIsVolatile = 5,
        StandardName = 6,
        StandardDisplayName = 7,
        StandardEditName = 8,
        StandardCopyName = 9,
        StandardDescription = 10,
        StandardAllocatedSize = 11,
        StandardIcon = 12,
        StandardSymbolicIcon = 13,
        StandardContentType = 14,
        StandardFastContentType = 15,
        StandardSize = 16,
        StandardSymlinkTarget = 17,
        StandardTargetUri = 18,
        StandardSortOrder = 19,

        EtagValue = 40,

        IdFile = 60,
        IdFilesystem = 61,

        AccessCanTrash = 100,
        AccessCanRead = 101,
        AccessCanWrite = 102,
        AccessCanExecute = 103,
        AccessCanDelete = 104,
        AccessCanRename = 105,

        MountableCanMount = 130,
        MountableCanUnmount = 131,
        MountableCanEject = 132,
        MountableUnixDevice = 133,
        MountableUnixDeviceFile = 134,
        MountableHalUdi = 135,
        MountableCanStart = 136,
        MountableCanStartDegraded = 137,
        MountableCanStop = 138,
        MountableStartStopType = 139,
        MountableCanPoll = 140,
        MountableIsMediaCheckAutomatic = 141,

        TimeModified = 200,
        TimeModifiedUsec = 201,
        TimeAccess = 202,
        TimeAccessUsec = 203,
        TimeChanged = 204,
        TimeChangedUsec = 205,
        TimeCreated = 206,
        TimeCreatedUsec = 207,

        OwnerUser = 300,
        OwnerUserReal = 301,
        OwnerGroup = 302,

        UnixDevice = 330,
        UnixInode = 331,
        UnixMode = 332,
        UnixNlink = 333,
        UnixUID = 334,
        UnixGID = 335,
        UnixRdev = 336,
        UnixBlockSize = 337,
        UnixBlocks = 338,
        UnixIsMountPoint = 339,

        DosIsArchive = 360,
        DosIsSystem = 361,

        ThumbnailPath = 390,
        ThumbnailFailed = 391,
        ThumbnailIsValid = 392,

        PreviewIcon = 420,

        FileSystemSize = 440,
        FileSystemFree = 441,
        FileSystemUsed = 442,
        FileSystemType = 443,
        FileSystemReadOnly = 444,
        FileSystemUsePreview = 445,
        FileSystemRemote = 446,

        GvfsBackend = 470,

        SelinuxContext = 490,

        TrashItemCount = 510,
        TrashOrigPath = 511,
        TrashDeletionDate = 512,

        RecentModified = 540,

        CustomStart = 1000,
    };

public:
    DFileInfo();
    explicit DFileInfo(const QUrl &uri);
    explicit DFileInfo(const DFileInfo &info);
    virtual ~DFileInfo();
    DFileInfo &operator=(const DFileInfo &info);

    QVariant attribute(AttributeID id, bool &success) const;
    bool setAttribute(AttributeID id, const QVariant &value);
    bool hasAttribute(AttributeID id) const;
    bool removeAttribute(AttributeID id);
    QList<AttributeID> attributeIDList() const;

    QUrl uri() const;
    QString dump() const;
    QString attributeName(AttributeID id) const;

    DFMIOError lastError() const;

protected:
    QSharedDataPointer<DFileInfoPrivate> d_ptr;
};

END_IO_NAMESPACE

#endif // DFILEINFO_H
