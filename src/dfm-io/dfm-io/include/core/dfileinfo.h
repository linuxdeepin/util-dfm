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

class DFileInfo : public QEnableSharedFromThis<DFileInfo>
{
public:
    enum class DFileType : uint16_t {
        kUnknown = 0,   // File's type is unknown.
        kRegular = 1,   // File handle represents a regular file.
        kDirectory = 2,   // File handle represents a directory.
        kSymbolicLink = 3,   // File handle represents a symbolic link (Unix systems).
        kSpecial = 4,   // File is a "special" file, such as a socket, fifo, block device, or character device.
        kShortcut = 5,   // File is a shortcut (Windows systems).
        kMountable = 6,   // File is a mountable location.

        // Reserved
        kUserType = 0x100
    };

    enum class DFileAttributeType : uint8_t {
        kTypeInvalid = 0,   // Indicates an invalid or uninitialized type
        kTypeString = 1,   // A null terminated UTF8 string
        kTypeByteString = 2,   // A zero terminated string of non-zero bytes
        kTypeBool = 3,   // A boolean value
        kTypeUInt32 = 4,   // An unsigned 4-byte/32-bit integer
        kTypeInt32 = 5,   // A signed 4-byte/32-bit integer
        kTypeUInt64 = 6,   // An unsigned 8-byte/64-bit integer
        kTypeInt64 = 7,   // A signed 8-byte/64-bit integer
        kTypeObject = 8,   // A Object
        kTypeStringV = 9   // A NULL terminated char **
    };

    enum class FileQueryInfoFlags : uint8_t {
        kTypeNone,
        kTypeNoFollowSymlinks
    };

    enum class AttributeID : uint16_t {
        kStandardType = 0,   // uint32
        kStandardIsHidden = 1,   // boolean
        kStandardIsBackup = 2,   // boolean
        kStandardIsSymlink = 3,   // boolean
        kStandardIsVirtual = 4,   // boolean
        kStandardIsVolatile = 5,   // boolean
        kStandardName = 6,   // byte string
        kStandardDisplayName = 7,   // string
        kStandardEditName = 8,   // string
        kStandardCopyName = 9,   // string
        kStandardIcon = 10,   // QList<QString>
        kStandardSymbolicIcon = 11,   // QList<QString>
        kStandardContentType = 12,   // string
        kStandardFastContentType = 13,   // string
        kStandardSize = 14,   // uint64
        kStandardAllocatedSize = 15,   // uint64
        kStandardSymlinkTarget = 16,   // byte string
        kStandardTargetUri = 17,   // string
        kStandardSortOrder = 18,   // int32
        kStandardDescription = 19,   // string

        kEtagValue = 40,   // string

        kIdFile = 60,   // string
        kIdFilesystem = 61,   // string

        kAccessCanRead = 100,   // boolean
        kAccessCanWrite = 101,   // boolean
        kAccessCanExecute = 102,   // boolean
        kAccessCanDelete = 103,   // boolean
        kAccessCanTrash = 104,   // boolean
        kAccessCanRename = 105,   // boolean

        kMountableCanMount = 130,   // boolean
        kMountableCanUnmount = 131,   // boolean
        kMountableCanEject = 132,   // boolean
        kMountableUnixDevice = 133,   // uint32
        kMountableUnixDeviceFile = 134,   // string
        kMountableHalUdi = 135,   // string
        kMountableCanPoll = 136,   // boolean
        kMountableIsMediaCheckAutomatic = 137,   // boolean
        kMountableCanStart = 138,   // boolean
        kMountableCanStartDegraded = 139,   // boolean
        kMountableCanStop = 140,   // boolean
        kMountableStartStopType = 141,   // uint32

        kTimeModified = 200,   // uint64
        kTimeModifiedUsec = 201,   // uint32
        kTimeAccess = 202,   // uint64
        kTimeAccessUsec = 203,   // uint32
        kTimeChanged = 204,   // uint64
        kTimeChangedUsec = 205,   // uint32
        kTimeCreated = 206,   // uint64
        kTimeCreatedUsec = 207,   // uint32

        kUnixDevice = 330,   // uint32
        kUnixInode = 331,   // uint64
        kUnixMode = 332,   // uint32
        kUnixNlink = 333,   // uint32
        kUnixUID = 334,   // uint32
        kUnixGID = 335,   // uint32
        kUnixRdev = 336,   // uint32
        kUnixBlockSize = 337,   // uint32
        kUnixBlocks = 338,   // uint64
        kUnixIsMountPoint = 339,   // boolean

        kDosIsArchive = 360,   // boolean
        kDosIsSystem = 361,   // boolean

        kOwnerUser = 300,   // string
        kOwnerUserReal = 301,   // string
        kOwnerGroup = 302,   // string

        kThumbnailPath = 390,   // byte string
        kThumbnailFailed = 391,   // boolean
        kThumbnailIsValid = 392,   // boolean

        kPreviewIcon = 420,   // object

        kFileSystemSize = 440,   // uint64
        kFileSystemFree = 441,   // uint64
        kFileSystemUsed = 442,   // uint64
        kFileSystemType = 443,   // string
        kFileSystemReadOnly = 444,   // boolean
        kFileSystemUsePreview = 445,   // uint32
        kFileSystemRemote = 446,   // boolean

        kGvfsBackend = 470,   // string

        kSelinuxContext = 490,   // string

        kTrashItemCount = 510,   // uint32
        kTrashDeletionDate = 511,   // string
        kTrashOrigPath = 512,   // string

        kRecentModified = 540,   // uint64

        kCustomStart = 600,

        kStandardIsFile = 610,
        kStandardIsDir = 611,
        kStandardIsRoot = 612,
        kStandardSuffix = 613,
        kStandardCompleteSuffix = 614,
        kStandardFilePath = 615,
        kStandardParentPath = 616,
        kStandardBaseName = 617,
        kStandardFileName = 618,
        kStandardCompleteBaseName = 619,

        kAttributeIDMax = 999,
    };

    enum class MediaType : uint8_t {
        kGeneral,
        kVideo,
        kAudio,
        kText,
        kOther,
        kImage,
        kMenu,

        kMax,
    };

    enum class AttributeExtendID : uint8_t {
        kExtendWordSize,   // xattr::word-size
        kExtendMediaDuration,   // xattr::media-duration
        kExtendMediaWidth,   // xattr::media-width
        kExtendMediaHeight,   // xattr::media-height
    };

    using AttributeInfoMap = std::unordered_map<DFileInfo::AttributeID, std::tuple<std::string, QVariant>>;
    static AttributeInfoMap attributeInfoMap;

    // callback, use function pointer
    using QueryInfoAsyncCallback = std::function<void(bool, void *)>;
    using AttributeExtendFuncCallback = std::function<void(bool, QMap<AttributeExtendID, QVariant>)>;

    // register function, use std::function
    using AttributeFunc = std::function<QVariant(DFileInfo::AttributeID, bool *)>;
    using SetAttributeFunc = std::function<bool(DFileInfo::AttributeID, const QVariant &)>;
    using HasAttributeFunc = std::function<bool(DFileInfo::AttributeID)>;
    using RemoveAttributeFunc = std::function<bool(DFileInfo::AttributeID)>;
    using AttributeListFunc = std::function<QList<DFileInfo::AttributeID>()>;
    using ExistsFunc = std::function<bool()>;
    using RefreshFunc = std::function<bool()>;
    using ClearCacheFunc = std::function<bool()>;
    using SetCustomAttributeFunc = std::function<bool(const char *, const DFileAttributeType, const void *, const FileQueryInfoFlags)>;
    using CustomAttributeFunc = std::function<QVariant(const char *, const DFileAttributeType)>;
    using LastErrorFunc = std::function<DFMIOError()>;
    using QueryInfoAsyncFunc = std::function<void(int, QueryInfoAsyncCallback, void *)>;

public:
    DFileInfo();
    explicit DFileInfo(const QUrl &uri, const char *attributes = "*", const FileQueryInfoFlags flag = FileQueryInfoFlags::kTypeNone);
    explicit DFileInfo(const DFileInfo &info);
    virtual ~DFileInfo();
    DFileInfo &operator=(const DFileInfo &info);

    DFM_VIRTUAL void queryInfoAsync(int ioPriority = 0, QueryInfoAsyncCallback func = nullptr, void *userData = nullptr) const;

    DFM_VIRTUAL QVariant attribute(DFileInfo::AttributeID id, bool *success = nullptr) const;
    DFM_VIRTUAL bool setAttribute(DFileInfo::AttributeID id, const QVariant &value);
    DFM_VIRTUAL bool hasAttribute(DFileInfo::AttributeID id) const;
    DFM_VIRTUAL bool removeAttribute(DFileInfo::AttributeID id);
    DFM_VIRTUAL QList<DFileInfo::AttributeID> attributeIDList() const;

    DFM_VIRTUAL bool exists() const;
    DFM_VIRTUAL bool refresh();
    DFM_VIRTUAL bool clearCache();
    DFM_VIRTUAL DFile::Permissions permissions();

    // custom attribute
    DFM_VIRTUAL bool setCustomAttribute(const char *key, const DFileAttributeType type, const void *value, const FileQueryInfoFlags flag = FileQueryInfoFlags::kTypeNone);
    DFM_VIRTUAL QVariant customAttribute(const char *key, const DFileAttributeType type);

    // extend attribute
    void attributeExtend(MediaType type, QList<AttributeExtendID> ids, AttributeExtendFuncCallback callback = nullptr);
    bool cancelAttributeExtend();

    DFM_VIRTUAL DFMIOError lastError() const;

    // register
    void registerAttribute(const AttributeFunc &func);
    void registerSetAttribute(const SetAttributeFunc &func);
    void registerHasAttribute(const HasAttributeFunc &func);
    void registerRemoveAttribute(const RemoveAttributeFunc &func);
    void registerAttributeList(const AttributeListFunc &func);
    void registerExists(const ExistsFunc &func);
    void registerRefresh(const RefreshFunc &func);
    void registerClearCache(const ClearCacheFunc &func);
    void registerPermissions(const DFile::PermissionFunc &func);
    void registerSetCustomAttribute(const SetCustomAttributeFunc &func);
    void registerCustomAttribute(const CustomAttributeFunc &func);
    void registerLastError(const LastErrorFunc &func);
    void registerQueryInfoAsync(const QueryInfoAsyncFunc &func);

    QUrl uri() const;
    char *queryAttributes() const;
    DFileInfo::FileQueryInfoFlags queryInfoFlag() const;
    QString dump() const;

private:
    QSharedPointer<DFileInfoPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DFILEINFO_H
