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
#include "dfile.h"

#include <QUrl>
#include <QSharedData>
#include <QSharedPointer>

#include <functional>
#include <unordered_map>

BEGIN_IO_NAMESPACE

class DFileInfoPrivate;

class DFileInfo
{
public:
    enum class DFileType : uint16_t {
        Unknown = 0,   // File's type is unknown.
        Regular = 1,   // File handle represents a regular file.
        Directory = 2,   // File handle represents a directory.
        SymbolicLink = 3,   // File handle represents a symbolic link (Unix systems).
        Special = 4,   // File is a "special" file, such as a socket, fifo, block device, or character device.
        Shortcut = 5,   // File is a shortcut (Windows systems).
        Mountable = 6,   // File is a mountable location.

        // Reserved
        UserType = 0x100
    };

    enum class AttributeID : uint16_t {
        StandardType = 0,   // uint32
        StandardIsHidden = 1,   // boolean
        StandardIsBackup = 2,   // boolean
        StandardIsSymlink = 3,   // boolean
        StandardIsVirtual = 4,   // boolean
        StandardIsVolatile = 5,   // boolean
        StandardName = 6,   // byte string
        StandardDisplayName = 7,   // string
        StandardEditName = 8,   // string
        StandardCopyName = 9,   // string
        StandardIcon = 10,   // QList<QString>
        StandardSymbolicIcon = 11,   // QList<QString>
        StandardContentType = 12,   // string
        StandardFastContentType = 13,   // string
        StandardSize = 14,   // uint64
        StandardAllocatedSize = 15,   // uint64
        StandardSymlinkTarget = 16,   // byte string
        StandardTargetUri = 17,   // string
        StandardSortOrder = 18,   // int32
        StandardDescription = 19,   // string

        EtagValue = 40,   // string

        IdFile = 60,   // string
        IdFilesystem = 61,   // string

        AccessCanRead = 100,   // boolean
        AccessCanWrite = 101,   // boolean
        AccessCanExecute = 102,   // boolean
        AccessCanDelete = 103,   // boolean
        AccessCanTrash = 104,   // boolean
        AccessCanRename = 105,   // boolean

        MountableCanMount = 130,   // boolean
        MountableCanUnmount = 131,   // boolean
        MountableCanEject = 132,   // boolean
        MountableUnixDevice = 133,   // uint32
        MountableUnixDeviceFile = 134,   // string
        MountableHalUdi = 135,   // string
        MountableCanPoll = 136,   // boolean
        MountableIsMediaCheckAutomatic = 137,   // boolean
        MountableCanStart = 138,   // boolean
        MountableCanStartDegraded = 139,   // boolean
        MountableCanStop = 140,   // boolean
        MountableStartStopType = 141,   // uint32

        TimeModified = 200,   // uint64
        TimeModifiedUsec = 201,   // uint32
        TimeAccess = 202,   // uint64
        TimeAccessUsec = 203,   // uint32
        TimeChanged = 204,   // uint64
        TimeChangedUsec = 205,   // uint32
        TimeCreated = 206,   // uint64
        TimeCreatedUsec = 207,   // uint32

        UnixDevice = 330,   // uint32
        UnixInode = 331,   // uint64
        UnixMode = 332,   // uint32
        UnixNlink = 333,   // uint32
        UnixUID = 334,   // uint32
        UnixGID = 335,   // uint32
        UnixRdev = 336,   // uint32
        UnixBlockSize = 337,   // uint32
        UnixBlocks = 338,   // uint64
        UnixIsMountPoint = 339,   // boolean

        DosIsArchive = 360,   // boolean
        DosIsSystem = 361,   // boolean

        OwnerUser = 300,   // string
        OwnerUserReal = 301,   // string
        OwnerGroup = 302,   // string

        ThumbnailPath = 390,   // byte string
        ThumbnailFailed = 391,   // boolean
        ThumbnailIsValid = 392,   // boolean

        PreviewIcon = 420,   // object

        FileSystemSize = 440,   // uint64
        FileSystemFree = 441,   // uint64
        FileSystemUsed = 442,   // uint64
        FileSystemType = 443,   // string
        FileSystemReadOnly = 444,   // boolean
        FileSystemUsePreview = 445,   // uint32
        FileSystemRemote = 446,   // boolean

        GvfsBackend = 470,   // string

        SelinuxContext = 490,   // string

        TrashItemCount = 510,   // uint32
        TrashDeletionDate = 511,   // string
        TrashOrigPath = 512,   // string

        RecentModified = 540,   // uint64

        ExtendWordSize = 550,   // uint32
        ExtendMediaDuration = 551,   // uint32, ms

        CustomStart = 600,

        StandardIsFile = 610,
        StandardIsDir = 611,
        StandardIsRoot = 612,
        StandardSuffix = 613,
        StandardCompleteSuffix = 614,
        StandardFilePath = 615,
        StandardParentPath = 616,
        StandardBaseName = 617,
        StandardFileName = 618,

        AttributeIDMax = 999,
    };

    using AttributeInfoMap = std::unordered_map<DFileInfo::AttributeID, std::tuple<std::string, QVariant>>;
    static AttributeInfoMap attributeInfoMap;

    using AttributeFunc = std::function<QVariant(DFileInfo::AttributeID, bool *)>;
    using SetAttributeFunc = std::function<bool(DFileInfo::AttributeID, const QVariant &)>;
    using HasAttributeFunc = std::function<bool(DFileInfo::AttributeID)>;
    using RemoveAttributeFunc = std::function<bool(DFileInfo::AttributeID)>;
    using AttributeListFunc = std::function<QList<DFileInfo::AttributeID>()>;
    using ExistsFunc = std::function<bool()>;
    using FlushFunc = std::function<bool()>;
    using LastErrorFunc = std::function<DFMIOError()>;

public:
    DFileInfo();
    explicit DFileInfo(const QUrl &uri);
    explicit DFileInfo(const DFileInfo &info);
    virtual ~DFileInfo();
    DFileInfo &operator=(const DFileInfo &info);

    DFM_VIRTUAL QVariant attribute(DFileInfo::AttributeID id, bool *success = nullptr) const;
    DFM_VIRTUAL bool setAttribute(DFileInfo::AttributeID id, const QVariant &value);
    DFM_VIRTUAL bool hasAttribute(DFileInfo::AttributeID id) const;
    DFM_VIRTUAL bool removeAttribute(DFileInfo::AttributeID id);
    DFM_VIRTUAL QList<DFileInfo::AttributeID> attributeIDList() const;
    DFM_VIRTUAL bool exists() const;
    DFM_VIRTUAL bool flush();
    DFM_VIRTUAL DFile::Permissions permissions();

    DFM_VIRTUAL DFMIOError lastError() const;

    // register
    void registerAttribute(const AttributeFunc &func);
    void registerSetAttribute(const SetAttributeFunc &func);
    void registerHasAttribute(const HasAttributeFunc &func);
    void registerRemoveAttribute(const RemoveAttributeFunc &func);
    void registerAttributeList(const AttributeListFunc &func);
    void registerExists(const ExistsFunc &func);
    void registerFlush(const FlushFunc &func);
    void registerPermissions(const DFile::PermissionFunc &func);
    void registerLastError(const LastErrorFunc &func);

    QUrl uri() const;
    QString dump() const;

private:
    QSharedPointer<DFileInfoPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DFILEINFO_H
