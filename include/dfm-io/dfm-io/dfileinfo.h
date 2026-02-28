// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILEINFO_H
#define DFILEINFO_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/error/error.h>
#include <dfm-io/dfile.h>

#include <QUrl>
#include <QSharedData>
#include <QSharedPointer>
#include <QtConcurrent>

#include <functional>
#include <unordered_map>

BEGIN_IO_NAMESPACE

class DFileFuture;
class DFileInfoPrivate;
class DFileInfo
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
        kOriginalUri = 20,

        kEtagValue = 40,   // string

        kIdFile = 60,   // string
        kIdFilesystem = 61,   // string

        kAccessCanRead = 100,   // boolean
        kAccessCanWrite = 101,   // boolean
        kAccessCanExecute = 102,   // boolean
        kAccessCanDelete = 103,   // boolean
        kAccessCanTrash = 104,   // boolean
        kAccessCanRename = 105,   // boolean
        kAccessPermissions = 106,   // DFile::Permissions

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
        kTrashOrigPath = 512,   // byte string

        kRecentModified = 540,   // int64

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

    // callback, use function pointer
    using InitQuerierAsyncCallback = std::function<void(bool, void *)>;
    using AttributeAsyncCallback = std::function<void(bool, void *, QVariant)>;
    using AttributeExtendFuncCallback = std::function<void(bool, QMap<AttributeExtendID, QVariant>)>;

public:
    explicit DFileInfo(const QUrl &uri, const char *attributes = "*", const FileQueryInfoFlags flag = FileQueryInfoFlags::kTypeNone);
    explicit DFileInfo(const QUrl &uri, void *fileInfo,
                       const char *attributes = "*", const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone);
    DFileInfo(const DFileInfo &other);
    DFileInfo &operator=(const DFileInfo &other);
    ~DFileInfo();

    bool initQuerier();
    QVariant attribute(DFileInfo::AttributeID id, bool *success = nullptr) const;
    void initQuerierAsync(int ioPriority = 0, InitQuerierAsyncCallback func = nullptr, void *userData = nullptr);
    void attributeAsync(DFileInfo::AttributeID id, bool *success = nullptr, int ioPriority = 0, AttributeAsyncCallback func = nullptr, void *userData = nullptr);

    [[nodiscard]] DFileFuture *initQuerierAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *attributeAsync(AttributeID id, int ioPriority, QObject *parent = nullptr) const;
    [[nodiscard]] DFileFuture *attributeAsync(const QByteArray &key, const DFileAttributeType type, int ioPriority, QObject *parent = nullptr) const;
    [[nodiscard]] DFileFuture *existsAsync(int ioPriority, QObject *parent = nullptr) const;
    [[nodiscard]] DFileFuture *refreshAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *permissionsAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] QFuture<void> refreshAsync();

    bool hasAttribute(DFileInfo::AttributeID id) const;
    bool exists() const;
    bool refresh();
    DFile::Permissions permissions() const;

    // custom attribute
    bool setCustomAttribute(const char *key, const DFileAttributeType type, const void *value, const FileQueryInfoFlags flag = FileQueryInfoFlags::kTypeNone);
    QVariant customAttribute(const char *key, const DFileAttributeType type) const;
    DFMIOError lastError() const;

    // extend attribute
    void attributeExtend(MediaType type, QList<AttributeExtendID> ids, AttributeExtendFuncCallback callback = nullptr);
    [[nodiscard]] DFileFuture *attributeExtend(MediaType type, QList<AttributeExtendID> ids, int ioPriority, QObject *parent = nullptr);
    bool cancelAttributeExtend();
    bool cancelAttributes();
    QUrl uri() const;
    char *queryAttributes() const;
    DFileInfo::FileQueryInfoFlags queryInfoFlag() const;
    QString dump() const;
    bool queryAttributeFinished() const;

private:
    mutable QSharedDataPointer<DFileInfoPrivate> d;
};

END_IO_NAMESPACE

#endif   // DFILEINFO_H
