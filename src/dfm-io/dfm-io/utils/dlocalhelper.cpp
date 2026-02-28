// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dlocalhelper.h"

#include <dfm-io/dfileinfo.h>
#include <dfm-io/dfmio_utils.h>

#include <QDebug>
#include <QCollator>
#include <QTime>

#include <gio/gfileinfo.h>

#include <sys/stat.h>
#include <sys/types.h>

USING_IO_NAMESPACE

namespace LocalFunc {

static bool isFile(GFileInfo *fileInfo)
{
    if (!fileInfo)
        return false;
    return g_file_info_get_file_type(fileInfo) == G_FILE_TYPE_REGULAR;
}

static bool isDir(GFileInfo *fileInfo)
{
    if (!fileInfo)
        return false;
    return g_file_info_get_file_type(fileInfo) == G_FILE_TYPE_DIRECTORY;
}

static bool isRoot(const QString &path)
{
    return path == "/";
}

static QString fileName(GFileInfo *fileInfo)
{
    if (!fileInfo)
        return QString();

    const char *name = g_file_info_get_name(fileInfo);
    return QString::fromLocal8Bit(name);
}

static QString baseName(GFileInfo *fileInfo)
{
    const QString &fullName = fileName(fileInfo);

    if (isDir(fileInfo))
        return fullName;

    int pos2 = fullName.indexOf(".");
    if (pos2 == -1)
        return fullName;
    else
        return fullName.left(pos2);
}

static QString completeBaseName(GFileInfo *fileInfo)
{
    const QString &fullName = fileName(fileInfo);

    if (isDir(fileInfo))
        return fullName;

    int pos2 = fullName.lastIndexOf(".");
    if (pos2 == -1)
        return fullName;
    else
        return fullName.left(pos2);
}

static QString suffix(GFileInfo *fileInfo)
{
    // path
    if (isDir(fileInfo))
        return "";

    const QString &fullName = fileName(fileInfo);

    int pos2 = fullName.lastIndexOf(".");
    if (pos2 == -1)
        return "";
    else
        return fullName.mid(pos2 + 1);
}

static QString completeSuffix(GFileInfo *fileInfo)
{
    if (isDir(fileInfo))
        return "";

    const QString &fullName = fileName(fileInfo);

    int pos2 = fullName.indexOf(".");
    if (pos2 == -1)
        return "";
    else
        return fullName.mid(pos2 + 1);
}

static QString filePath(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toStdString().c_str());

    g_autofree gchar *gpath = g_file_get_path(file);   // no blocking I/O
    if (gpath != nullptr)
        return QString::fromLocal8Bit(gpath);

    return "";
}

static QString parentPath(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toStdString().c_str());
    g_autoptr(GFile) fileParent = g_file_get_parent(file);   // no blocking I/O

    g_autofree gchar *gpath = g_file_get_path(fileParent);   // no blocking I/O
    if (gpath != nullptr)
        return QString::fromLocal8Bit(gpath);
    return "";
}
}   // LocalFunc

DLocalHelper::AttributeInfoMap &DLocalHelper::attributeInfoMapFunc()
{
    static AttributeInfoMap kAttributeInfoMap {
        { DFileInfo::AttributeID::kStandardType, std::make_tuple<std::string, QVariant>("standard::type", 0) },   // G_FILE_ATTRIBUTE_STANDARD_TYPE
        { DFileInfo::AttributeID::kStandardIsHidden, std::make_tuple<std::string, QVariant>("standard::is-hidden", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN
        { DFileInfo::AttributeID::kStandardIsBackup, std::make_tuple<std::string, QVariant>("standard::is-backup", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP
        { DFileInfo::AttributeID::kStandardIsSymlink, std::make_tuple<std::string, QVariant>("standard::is-symlink", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK
        { DFileInfo::AttributeID::kStandardIsVirtual, std::make_tuple<std::string, QVariant>("standard::is-virtual", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL
        { DFileInfo::AttributeID::kStandardIsVolatile, std::make_tuple<std::string, QVariant>("standard::is-volatile", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE
        { DFileInfo::AttributeID::kStandardName, std::make_tuple<std::string, QVariant>("standard::name", "") },   // G_FILE_ATTRIBUTE_STANDARD_NAME
        { DFileInfo::AttributeID::kStandardDisplayName, std::make_tuple<std::string, QVariant>("standard::display-name", "") },   // G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME
        { DFileInfo::AttributeID::kStandardEditName, std::make_tuple<std::string, QVariant>("standard::edit-name", "") },   // G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME
        { DFileInfo::AttributeID::kStandardCopyName, std::make_tuple<std::string, QVariant>("standard::copy-name", "") },   // G_FILE_ATTRIBUTE_STANDARD_COPY_NAME
        { DFileInfo::AttributeID::kStandardIcon, std::make_tuple<std::string, QVariant>("standard::icon", 0) },   // G_FILE_ATTRIBUTE_STANDARD_ICON
        { DFileInfo::AttributeID::kStandardSymbolicIcon, std::make_tuple<std::string, QVariant>("standard::symbolic-icon", 0) },   // G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON
        { DFileInfo::AttributeID::kStandardContentType, std::make_tuple<std::string, QVariant>("standard::content-type", "") },   // G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE
        { DFileInfo::AttributeID::kStandardFastContentType, std::make_tuple<std::string, QVariant>("standard::fast-content-type", "") },   // G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE
        { DFileInfo::AttributeID::kStandardSize, std::make_tuple<std::string, QVariant>("standard::size", 0) },   // G_FILE_ATTRIBUTE_STANDARD_SIZE
        { DFileInfo::AttributeID::kStandardAllocatedSize, std::make_tuple<std::string, QVariant>("standard::allocated-size", 0) },   // G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE
        { DFileInfo::AttributeID::kStandardSymlinkTarget, std::make_tuple<std::string, QVariant>("standard::symlink-target", "") },   // G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET
        { DFileInfo::AttributeID::kStandardTargetUri, std::make_tuple<std::string, QVariant>("standard::target-uri", "") },   // G_FILE_ATTRIBUTE_STANDARD_TARGET_URI
        { DFileInfo::AttributeID::kStandardSortOrder, std::make_tuple<std::string, QVariant>("standard::sort-order", 0) },   // G_FILE_ATTRIBUTE_STANDARD_SORT_ORDER
        { DFileInfo::AttributeID::kStandardDescription, std::make_tuple<std::string, QVariant>("standard::description", "") },   // G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION

        { DFileInfo::AttributeID::kEtagValue, std::make_tuple<std::string, QVariant>("etag::value", "") },   // G_FILE_ATTRIBUTE_ETAG_VALUE

        { DFileInfo::AttributeID::kIdFile, std::make_tuple<std::string, QVariant>("id::file", "") },   // G_FILE_ATTRIBUTE_ID_FILE
        { DFileInfo::AttributeID::kIdFilesystem, std::make_tuple<std::string, QVariant>("id::filesystem", "") },   // G_FILE_ATTRIBUTE_ID_FILESYSTEM

        { DFileInfo::AttributeID::kAccessCanRead, std::make_tuple<std::string, QVariant>("access::can-read", true) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_READ
        { DFileInfo::AttributeID::kAccessCanWrite, std::make_tuple<std::string, QVariant>("access::can-write", true) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE
        { DFileInfo::AttributeID::kAccessCanExecute, std::make_tuple<std::string, QVariant>("access::can-execute", true) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE
        { DFileInfo::AttributeID::kAccessCanDelete, std::make_tuple<std::string, QVariant>("access::can-delete", true) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE
        { DFileInfo::AttributeID::kAccessCanTrash, std::make_tuple<std::string, QVariant>("access::can-trash", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH
        { DFileInfo::AttributeID::kAccessCanRename, std::make_tuple<std::string, QVariant>("access::can-rename", true) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME

        { DFileInfo::AttributeID::kMountableCanMount, std::make_tuple<std::string, QVariant>("mountable::can-mount", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT
        { DFileInfo::AttributeID::kMountableCanUnmount, std::make_tuple<std::string, QVariant>("mountable::can-unmount", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT
        { DFileInfo::AttributeID::kMountableCanEject, std::make_tuple<std::string, QVariant>("mountable::can-eject", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT
        { DFileInfo::AttributeID::kMountableUnixDevice, std::make_tuple<std::string, QVariant>("mountable::unix-device", 0) },   // G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE
        { DFileInfo::AttributeID::kMountableUnixDeviceFile, std::make_tuple<std::string, QVariant>("mountable::unix-device-file", "") },   // G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE
        { DFileInfo::AttributeID::kMountableHalUdi, std::make_tuple<std::string, QVariant>("mountable::hal-udi", "") },   // G_FILE_ATTRIBUTE_MOUNTABLE_HAL_UDI
        { DFileInfo::AttributeID::kMountableCanPoll, std::make_tuple<std::string, QVariant>("mountable::can-poll", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_POLL
        { DFileInfo::AttributeID::kMountableIsMediaCheckAutomatic, std::make_tuple<std::string, QVariant>("mountable::is-media-check-automatic", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_IS_MEDIA_CHECK_AUTOMATIC
        { DFileInfo::AttributeID::kMountableCanStart, std::make_tuple<std::string, QVariant>("mountable::can-start", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START
        { DFileInfo::AttributeID::kMountableCanStartDegraded, std::make_tuple<std::string, QVariant>("mountable::can-start-degraded", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START_DEGRADED
        { DFileInfo::AttributeID::kMountableCanStop, std::make_tuple<std::string, QVariant>("mountable::can-stop", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_STOP
        { DFileInfo::AttributeID::kMountableStartStopType, std::make_tuple<std::string, QVariant>("mountable::start-stop-type", 0) },   // G_FILE_ATTRIBUTE_MOUNTABLE_START_STOP_TYPE

        { DFileInfo::AttributeID::kTimeModified, std::make_tuple<std::string, QVariant>("time::modified", 0) },   // G_FILE_ATTRIBUTE_TIME_MODIFIED
        { DFileInfo::AttributeID::kTimeModifiedUsec, std::make_tuple<std::string, QVariant>("time::modified-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC
        { DFileInfo::AttributeID::kTimeAccess, std::make_tuple<std::string, QVariant>("time::access", 0) },   // G_FILE_ATTRIBUTE_TIME_ACCESS
        { DFileInfo::AttributeID::kTimeAccessUsec, std::make_tuple<std::string, QVariant>("time::access-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_ACCESS_USEC
        { DFileInfo::AttributeID::kTimeChanged, std::make_tuple<std::string, QVariant>("time::changed", 0) },   // G_FILE_ATTRIBUTE_TIME_CHANGED
        { DFileInfo::AttributeID::kTimeChangedUsec, std::make_tuple<std::string, QVariant>("time::changed-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_CHANGED_USEC
        { DFileInfo::AttributeID::kTimeCreated, std::make_tuple<std::string, QVariant>("time::created", 0) },   // G_FILE_ATTRIBUTE_TIME_CREATED
        { DFileInfo::AttributeID::kTimeCreatedUsec, std::make_tuple<std::string, QVariant>("time::created-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_CREATED_USEC

        { DFileInfo::AttributeID::kUnixDevice, std::make_tuple<std::string, QVariant>("unix::device", 0) },   // G_FILE_ATTRIBUTE_UNIX_DEVICE
        { DFileInfo::AttributeID::kUnixInode, std::make_tuple<std::string, QVariant>("unix::inode", 0) },   // G_FILE_ATTRIBUTE_UNIX_INODE
        { DFileInfo::AttributeID::kUnixMode, std::make_tuple<std::string, QVariant>("unix::mode", 0) },   // G_FILE_ATTRIBUTE_UNIX_MODE
        { DFileInfo::AttributeID::kUnixNlink, std::make_tuple<std::string, QVariant>("unix::nlink", 0) },   // G_FILE_ATTRIBUTE_UNIX_NLINK
        { DFileInfo::AttributeID::kUnixUID, std::make_tuple<std::string, QVariant>("unix::uid", 0) },   // G_FILE_ATTRIBUTE_UNIX_UID
        { DFileInfo::AttributeID::kUnixGID, std::make_tuple<std::string, QVariant>("unix::gid", 0) },   // G_FILE_ATTRIBUTE_UNIX_GID
        { DFileInfo::AttributeID::kUnixRdev, std::make_tuple<std::string, QVariant>("unix::rdev", 0) },   // G_FILE_ATTRIBUTE_UNIX_RDEV
        { DFileInfo::AttributeID::kUnixBlockSize, std::make_tuple<std::string, QVariant>("unix::block-size", 0) },   // G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE
        { DFileInfo::AttributeID::kUnixBlocks, std::make_tuple<std::string, QVariant>("unix::blocks", 0) },   // G_FILE_ATTRIBUTE_UNIX_BLOCKS
        { DFileInfo::AttributeID::kUnixIsMountPoint, std::make_tuple<std::string, QVariant>("unix::is-mountpoint", false) },   // G_FILE_ATTRIBUTE_UNIX_IS_MOUNTPOINT

        { DFileInfo::AttributeID::kDosIsArchive, std::make_tuple<std::string, QVariant>("dos::is-archive", false) },   // G_FILE_ATTRIBUTE_DOS_IS_ARCHIVE
        { DFileInfo::AttributeID::kDosIsSystem, std::make_tuple<std::string, QVariant>("dos::is-system", false) },   // G_FILE_ATTRIBUTE_DOS_IS_SYSTEM

        { DFileInfo::AttributeID::kOwnerUser, std::make_tuple<std::string, QVariant>("owner::user", "") },   // G_FILE_ATTRIBUTE_OWNER_USER
        { DFileInfo::AttributeID::kOwnerUserReal, std::make_tuple<std::string, QVariant>("owner::user-real", "") },   // G_FILE_ATTRIBUTE_OWNER_USER_REAL
        { DFileInfo::AttributeID::kOwnerGroup, std::make_tuple<std::string, QVariant>("owner::group", "") },   // G_FILE_ATTRIBUTE_OWNER_GROUP

        { DFileInfo::AttributeID::kThumbnailPath, std::make_tuple<std::string, QVariant>("thumbnail::path", "") },   // G_FILE_ATTRIBUTE_THUMBNAIL_PATH
        { DFileInfo::AttributeID::kThumbnailFailed, std::make_tuple<std::string, QVariant>("thumbnail::failed", false) },   // G_FILE_ATTRIBUTE_THUMBNAILING_FAILED
        { DFileInfo::AttributeID::kThumbnailIsValid, std::make_tuple<std::string, QVariant>("thumbnail::is-valid", false) },   // G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID

        { DFileInfo::AttributeID::kPreviewIcon, std::make_tuple<std::string, QVariant>("preview::icon", 0) },   // G_FILE_ATTRIBUTE_PREVIEW_ICON

        { DFileInfo::AttributeID::kFileSystemSize, std::make_tuple<std::string, QVariant>("filesystem::size", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_SIZE
        { DFileInfo::AttributeID::kFileSystemFree, std::make_tuple<std::string, QVariant>("filesystem::free", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_FREE
        { DFileInfo::AttributeID::kFileSystemUsed, std::make_tuple<std::string, QVariant>("filesystem::used", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_USED
        { DFileInfo::AttributeID::kFileSystemType, std::make_tuple<std::string, QVariant>("filesystem::type", "") },   // G_FILE_ATTRIBUTE_FILESYSTEM_TYPE
        { DFileInfo::AttributeID::kFileSystemReadOnly, std::make_tuple<std::string, QVariant>("filesystem::readonly", false) },   // G_FILE_ATTRIBUTE_FILESYSTEM_READONLY
        { DFileInfo::AttributeID::kFileSystemUsePreview, std::make_tuple<std::string, QVariant>("filesystem::use-preview", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_USE_PREVIEW
        { DFileInfo::AttributeID::kFileSystemRemote, std::make_tuple<std::string, QVariant>("filesystem::remote", false) },   // G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE

        { DFileInfo::AttributeID::kGvfsBackend, std::make_tuple<std::string, QVariant>("gvfs::backend", "") },   // G_FILE_ATTRIBUTE_GVFS_BACKEND

        { DFileInfo::AttributeID::kSelinuxContext, std::make_tuple<std::string, QVariant>("selinux::context", "") },   // G_FILE_ATTRIBUTE_SELINUX_CONTEXT

        { DFileInfo::AttributeID::kTrashItemCount, std::make_tuple<std::string, QVariant>("trash::item-count", 0) },   // G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT
        { DFileInfo::AttributeID::kTrashDeletionDate, std::make_tuple<std::string, QVariant>("trash::deletion-date", "") },   // G_FILE_ATTRIBUTE_TRASH_DELETION_DATE
        { DFileInfo::AttributeID::kTrashOrigPath, std::make_tuple<std::string, QVariant>("trash::orig-path", "") },   // G_FILE_ATTRIBUTE_TRASH_ORIG_PATH

        { DFileInfo::AttributeID::kRecentModified, std::make_tuple<std::string, QVariant>("recent::modified", 0) },   // G_FILE_ATTRIBUTE_RECENT_MODIFIED

        { DFileInfo::AttributeID::kCustomStart, std::make_tuple<std::string, QVariant>("custom-start", 0) },

        { DFileInfo::AttributeID::kStandardIsFile, std::make_tuple<std::string, QVariant>("standard::is-file", false) },
        { DFileInfo::AttributeID::kStandardIsDir, std::make_tuple<std::string, QVariant>("standard::is-dir", false) },
        { DFileInfo::AttributeID::kStandardIsRoot, std::make_tuple<std::string, QVariant>("standard::is-root", false) },
        { DFileInfo::AttributeID::kStandardSuffix, std::make_tuple<std::string, QVariant>("standard::suffix", "") },
        { DFileInfo::AttributeID::kStandardCompleteSuffix, std::make_tuple<std::string, QVariant>("standard::complete-suffix", "") },
        { DFileInfo::AttributeID::kStandardFilePath, std::make_tuple<std::string, QVariant>("standard::file-path", "") },
        { DFileInfo::AttributeID::kStandardParentPath, std::make_tuple<std::string, QVariant>("standard::parent-path", "") },
        { DFileInfo::AttributeID::kStandardBaseName, std::make_tuple<std::string, QVariant>("standard::base-name", "") },
        { DFileInfo::AttributeID::kStandardFileName, std::make_tuple<std::string, QVariant>("standard::file-name", "") },
        { DFileInfo::AttributeID::kStandardCompleteBaseName, std::make_tuple<std::string, QVariant>("standard::complete-base-name", "") },
    };
    return kAttributeInfoMap;
}

QSharedPointer<DFileInfo> DLocalHelper::createFileInfoByUri(const QUrl &uri, const char *attributes /*= "*"*/,
                                                            const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/)
{
    return QSharedPointer<DFileInfo>(new DFileInfo(uri, attributes, flag));
}

QSharedPointer<DFileInfo> DLocalHelper::createFileInfoByUri(const QUrl &uri, GFileInfo *gfileInfo, const char *attributes, const DFileInfo::FileQueryInfoFlags flag)
{
    return QSharedPointer<DFileInfo>(new DFileInfo(uri, gfileInfo, attributes, flag));
}

QVariant DLocalHelper::attributeFromGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, DFMIOErrorCode &errorcode)
{
    if (!gfileinfo)
        return QVariant();

    // check custom attribute
    if (id > DFileInfo::AttributeID::kCustomStart) {
        return QVariant();
    }

    switch (id) {
    // uint32_t
    case DFileInfo::AttributeID::kStandardType:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_TYPE, errorcode);
    case DFileInfo::AttributeID::kMountableUnixDevice:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE, errorcode);
    case DFileInfo::AttributeID::kMountableStartStopType:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_START_STOP_TYPE, errorcode);
    case DFileInfo::AttributeID::kTimeModifiedUsec:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC, errorcode);
    case DFileInfo::AttributeID::kTimeAccessUsec:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_TIME_ACCESS_USEC, errorcode);
    case DFileInfo::AttributeID::kTimeChangedUsec:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_TIME_CHANGED_USEC, errorcode);
    case DFileInfo::AttributeID::kUnixDevice:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_DEVICE, errorcode);
    case DFileInfo::AttributeID::kUnixMode:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_MODE, errorcode);
    case DFileInfo::AttributeID::kUnixNlink:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_NLINK, errorcode);
    case DFileInfo::AttributeID::kUnixUID:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_UID, errorcode);
    case DFileInfo::AttributeID::kUnixGID:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_GID, errorcode);
    case DFileInfo::AttributeID::kUnixRdev:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_RDEV, errorcode);
    case DFileInfo::AttributeID::kUnixBlockSize:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE, errorcode);
    case DFileInfo::AttributeID::kFileSystemUsePreview:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_USE_PREVIEW, errorcode);
    case DFileInfo::AttributeID::kTrashItemCount:
        return getGFileInfoUint32(gfileinfo, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT, errorcode);
    // int32_t
    case DFileInfo::AttributeID::kStandardSortOrder:
        return getGFileInfoInt32(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_SORT_ORDER, errorcode);
    // uint64_t
    case DFileInfo::AttributeID::kStandardSize:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_SIZE, errorcode);
    case DFileInfo::AttributeID::kStandardAllocatedSize:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE, errorcode);
    case DFileInfo::AttributeID::kTimeModified:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED, errorcode);
    case DFileInfo::AttributeID::kTimeAccess:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_TIME_ACCESS, errorcode);
    case DFileInfo::AttributeID::kTimeChanged:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_TIME_CHANGED, errorcode);
    case DFileInfo::AttributeID::kUnixInode:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_UNIX_INODE, errorcode);
    case DFileInfo::AttributeID::kUnixBlocks:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_UNIX_BLOCKS, errorcode);
    case DFileInfo::AttributeID::kFileSystemSize:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE, errorcode);
    case DFileInfo::AttributeID::kFileSystemFree:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, errorcode);
    case DFileInfo::AttributeID::kFileSystemUsed:
        return getGFileInfoUint64(gfileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_USED, errorcode);
    // Int64_t
    case DFileInfo::AttributeID::kRecentModified:
        return getGFileInfoInt64(gfileinfo, G_FILE_ATTRIBUTE_RECENT_MODIFIED, errorcode);
    // bool
    case DFileInfo::AttributeID::kStandardIsHidden:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN, errorcode);
    case DFileInfo::AttributeID::kStandardIsBackup:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP, errorcode);
    case DFileInfo::AttributeID::kStandardIsSymlink:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK, errorcode);
    case DFileInfo::AttributeID::kStandardIsVirtual:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL, errorcode);
    case DFileInfo::AttributeID::kStandardIsVolatile:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE, errorcode);
    case DFileInfo::AttributeID::kAccessCanRead:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, errorcode);
    case DFileInfo::AttributeID::kAccessCanWrite:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, errorcode);
    case DFileInfo::AttributeID::kAccessCanExecute:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE, errorcode);
    case DFileInfo::AttributeID::kAccessCanDelete:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, errorcode);
    case DFileInfo::AttributeID::kAccessCanTrash:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, errorcode);
    case DFileInfo::AttributeID::kAccessCanRename:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, errorcode);
    case DFileInfo::AttributeID::kMountableCanMount:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT, errorcode);
    case DFileInfo::AttributeID::kMountableCanUnmount:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT, errorcode);
    case DFileInfo::AttributeID::kMountableCanEject:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT, errorcode);
    case DFileInfo::AttributeID::kMountableCanPoll:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_POLL, errorcode);
    case DFileInfo::AttributeID::kMountableIsMediaCheckAutomatic:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_IS_MEDIA_CHECK_AUTOMATIC, errorcode);
    case DFileInfo::AttributeID::kMountableCanStart:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START, errorcode);
    case DFileInfo::AttributeID::kMountableCanStartDegraded:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START_DEGRADED, errorcode);
    case DFileInfo::AttributeID::kMountableCanStop:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_STOP, errorcode);
    case DFileInfo::AttributeID::kUnixIsMountPoint:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_UNIX_IS_MOUNTPOINT, errorcode);
    case DFileInfo::AttributeID::kDosIsArchive:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_DOS_IS_ARCHIVE, errorcode);
    case DFileInfo::AttributeID::kDosIsSystem:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_DOS_IS_SYSTEM, errorcode);
    case DFileInfo::AttributeID::kFileSystemReadOnly:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY, errorcode);
    case DFileInfo::AttributeID::kFileSystemRemote:
        return getGFileInfoBool(gfileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE, errorcode);
    // byte string
    case DFileInfo::AttributeID::kStandardName:
        return getGFileInfoByteString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_NAME, errorcode);
    case DFileInfo::AttributeID::kStandardSymlinkTarget:
        return getGFileInfoByteString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET, errorcode);
    case DFileInfo::AttributeID::kTrashOrigPath:
        return getGFileInfoByteString(gfileinfo, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH, errorcode);
    case DFileInfo::AttributeID::kThumbnailPath:
        return getGFileInfoByteString(gfileinfo, G_FILE_ATTRIBUTE_THUMBNAIL_PATH, errorcode);
    // string
    case DFileInfo::AttributeID::kStandardDisplayName:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, errorcode);
    case DFileInfo::AttributeID::kStandardEditName:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME, errorcode);
    case DFileInfo::AttributeID::kStandardCopyName:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_COPY_NAME, errorcode);
    case DFileInfo::AttributeID::kStandardContentType:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, errorcode);
    case DFileInfo::AttributeID::kStandardFastContentType:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE, errorcode);
    case DFileInfo::AttributeID::kStandardTargetUri:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, errorcode);
    case DFileInfo::AttributeID::kStandardDescription:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION, errorcode);
    case DFileInfo::AttributeID::kEtagValue:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_ETAG_VALUE, errorcode);
    case DFileInfo::AttributeID::kIdFile:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_ID_FILE, errorcode);
    case DFileInfo::AttributeID::kIdFilesystem:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_ID_FILESYSTEM, errorcode);
    case DFileInfo::AttributeID::kMountableUnixDeviceFile:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE, errorcode);
    case DFileInfo::AttributeID::kMountableHalUdi:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_MOUNTABLE_HAL_UDI, errorcode);
    case DFileInfo::AttributeID::kOwnerUser:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_OWNER_USER, errorcode);
    case DFileInfo::AttributeID::kOwnerUserReal:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_OWNER_USER_REAL, errorcode);
    case DFileInfo::AttributeID::kOwnerGroup:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_OWNER_GROUP, errorcode);
    case DFileInfo::AttributeID::kFileSystemType:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE, errorcode);
    case DFileInfo::AttributeID::kGvfsBackend:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_GVFS_BACKEND, errorcode);
    case DFileInfo::AttributeID::kSelinuxContext:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_SELINUX_CONTEXT, errorcode);
    case DFileInfo::AttributeID::kTrashDeletionDate:
        return getGFileInfoString(gfileinfo, G_FILE_ATTRIBUTE_TRASH_DELETION_DATE, errorcode);
    // object
    case DFileInfo::AttributeID::kStandardIcon:
        return getGFileInfoIcon(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_ICON, errorcode);
    case DFileInfo::AttributeID::kPreviewIcon:
        return getGFileInfoIcon(gfileinfo, G_FILE_ATTRIBUTE_PREVIEW_ICON, errorcode);
    case DFileInfo::AttributeID::kStandardSymbolicIcon:
        return getGFileInfoIcon(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON, errorcode);
    default:
        return QVariant();
    }
}

QVariant DLocalHelper::customAttributeFromPathAndInfo(const QString &path, GFileInfo *fileInfo, DFileInfo::AttributeID id)
{
    if (id < DFileInfo::AttributeID::kCustomStart)
        return QVariant();

    switch (id) {
    case DFileInfo::AttributeID::kStandardIsFile: {
        return LocalFunc::isFile(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardIsDir: {
        return LocalFunc::isDir(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardIsRoot: {
        return LocalFunc::isRoot(path);
    }
    case DFileInfo::AttributeID::kStandardSuffix: {
        return LocalFunc::suffix(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardCompleteSuffix: {
        return LocalFunc::completeSuffix(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardFilePath: {
        return LocalFunc::filePath(path);
    }
    case DFileInfo::AttributeID::kStandardParentPath: {
        return LocalFunc::parentPath(path);
    }
    case DFileInfo::AttributeID::kStandardBaseName: {
        return LocalFunc::baseName(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardFileName: {
        return LocalFunc::fileName(fileInfo);
    }
    case DFileInfo::AttributeID::kStandardCompleteBaseName: {
        return LocalFunc::completeBaseName(fileInfo);
    }
    default:
        return QVariant();
    }
}

QVariant DLocalHelper::customAttributeFromPath(const QString &path, DFileInfo::AttributeID id)
{
    Q_UNUSED(path)
    Q_UNUSED(id)
    return QVariant();
}

bool DLocalHelper::setAttributeByGFile(GFile *gfile, DFileInfo::AttributeID id, const QVariant &value, GError **gerror)
{
    if (!gfile) {
        return false;
    }

    switch (id) {
    // uint32_t
    case DFileInfo::AttributeID::kStandardType:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_STANDARD_TYPE, value, gerror);
    case DFileInfo::AttributeID::kMountableUnixDevice:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE, value, gerror);
    case DFileInfo::AttributeID::kMountableStartStopType:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_START_STOP_TYPE, value, gerror);
    case DFileInfo::AttributeID::kTimeModifiedUsec:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC, value, gerror);
    case DFileInfo::AttributeID::kTimeAccessUsec:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_TIME_ACCESS_USEC, value, gerror);
    case DFileInfo::AttributeID::kTimeChangedUsec:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_TIME_CHANGED_USEC, value, gerror);
    case DFileInfo::AttributeID::kTimeCreatedUsec:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_TIME_CREATED_USEC, value, gerror);
    case DFileInfo::AttributeID::kUnixDevice:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_UNIX_DEVICE, value, gerror);
    case DFileInfo::AttributeID::kUnixMode:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_UNIX_MODE, value, gerror);
    case DFileInfo::AttributeID::kUnixNlink:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_UNIX_NLINK, value, gerror);
    case DFileInfo::AttributeID::kUnixUID:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_UNIX_UID, value, gerror);
    case DFileInfo::AttributeID::kUnixGID:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_UNIX_GID, value, gerror);
    case DFileInfo::AttributeID::kUnixRdev:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_UNIX_RDEV, value, gerror);
    case DFileInfo::AttributeID::kUnixBlockSize:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE, value, gerror);
    case DFileInfo::AttributeID::kFileSystemUsePreview:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_USE_PREVIEW, value, gerror);
    case DFileInfo::AttributeID::kTrashItemCount:
        return setGFileInfoUint32(gfile, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT, value, gerror);
    // int32_t
    case DFileInfo::AttributeID::kStandardSortOrder:
        return setGFileInfoInt32(gfile, G_FILE_ATTRIBUTE_STANDARD_SORT_ORDER, value, gerror);
    // uint64_t
    case DFileInfo::AttributeID::kStandardSize:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE, value, gerror);
    case DFileInfo::AttributeID::kStandardAllocatedSize:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE, value, gerror);
    case DFileInfo::AttributeID::kTimeModified:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_TIME_MODIFIED, value, gerror);
    case DFileInfo::AttributeID::kTimeAccess:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_TIME_ACCESS, value, gerror);
    case DFileInfo::AttributeID::kTimeChanged:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_TIME_CHANGED, value, gerror);
    case DFileInfo::AttributeID::kTimeCreated:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_TIME_CREATED, value, gerror);
    case DFileInfo::AttributeID::kUnixInode:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_UNIX_INODE, value, gerror);
    case DFileInfo::AttributeID::kUnixBlocks:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_UNIX_BLOCKS, value, gerror);
    case DFileInfo::AttributeID::kFileSystemSize:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE, value, gerror);
    case DFileInfo::AttributeID::kFileSystemFree:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, value, gerror);
    case DFileInfo::AttributeID::kFileSystemUsed:
        return setGFileInfoUint64(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_USED, value, gerror);
    // int64_t
    case DFileInfo::AttributeID::kRecentModified:
        return setGFileInfoInt64(gfile, G_FILE_ATTRIBUTE_RECENT_MODIFIED, value, gerror);
    // bool
    case DFileInfo::AttributeID::kStandardIsHidden:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN, value, gerror);
    case DFileInfo::AttributeID::kStandardIsBackup:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP, value, gerror);
    case DFileInfo::AttributeID::kStandardIsSymlink:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK, value, gerror);
    case DFileInfo::AttributeID::kStandardIsVirtual:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL, value, gerror);
    case DFileInfo::AttributeID::kStandardIsVolatile:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE, value, gerror);
    case DFileInfo::AttributeID::kAccessCanRead:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, value, gerror);
    case DFileInfo::AttributeID::kAccessCanWrite:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, value, gerror);
    case DFileInfo::AttributeID::kAccessCanExecute:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE, value, gerror);
    case DFileInfo::AttributeID::kAccessCanDelete:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, value, gerror);
    case DFileInfo::AttributeID::kAccessCanTrash:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, value, gerror);
    case DFileInfo::AttributeID::kAccessCanRename:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, value, gerror);
    case DFileInfo::AttributeID::kMountableCanMount:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT, value, gerror);
    case DFileInfo::AttributeID::kMountableCanUnmount:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT, value, gerror);
    case DFileInfo::AttributeID::kMountableCanEject:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT, value, gerror);
    case DFileInfo::AttributeID::kMountableCanPoll:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_POLL, value, gerror);
    case DFileInfo::AttributeID::kMountableIsMediaCheckAutomatic:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_IS_MEDIA_CHECK_AUTOMATIC, value, gerror);
    case DFileInfo::AttributeID::kMountableCanStart:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START, value, gerror);
    case DFileInfo::AttributeID::kMountableCanStartDegraded:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START_DEGRADED, value, gerror);
    case DFileInfo::AttributeID::kMountableCanStop:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_CAN_STOP, value, gerror);
    case DFileInfo::AttributeID::kUnixIsMountPoint:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_UNIX_IS_MOUNTPOINT, value, gerror);
    case DFileInfo::AttributeID::kDosIsArchive:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_DOS_IS_ARCHIVE, value, gerror);
    case DFileInfo::AttributeID::kDosIsSystem:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_DOS_IS_SYSTEM, value, gerror);
    case DFileInfo::AttributeID::kFileSystemReadOnly:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY, value, gerror);
    case DFileInfo::AttributeID::kFileSystemRemote:
        return setGFileInfoBool(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE, value, gerror);
    // byte string
    case DFileInfo::AttributeID::kStandardName:
        return setGFileInfoByteString(gfile, G_FILE_ATTRIBUTE_STANDARD_NAME, value, gerror);
    case DFileInfo::AttributeID::kStandardSymlinkTarget:
        return setGFileInfoByteString(gfile, G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET, value, gerror);
    case DFileInfo::AttributeID::kThumbnailPath:
        return setGFileInfoByteString(gfile, G_FILE_ATTRIBUTE_THUMBNAIL_PATH, value, gerror);
    // string
    case DFileInfo::AttributeID::kStandardDisplayName:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, value, gerror);
    case DFileInfo::AttributeID::kStandardEditName:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME, value, gerror);
    case DFileInfo::AttributeID::kStandardCopyName:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_STANDARD_COPY_NAME, value, gerror);
    case DFileInfo::AttributeID::kStandardContentType:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, value, gerror);
    case DFileInfo::AttributeID::kStandardFastContentType:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE, value, gerror);
    case DFileInfo::AttributeID::kStandardTargetUri:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, value, gerror);
    case DFileInfo::AttributeID::kStandardDescription:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION, value, gerror);
    case DFileInfo::AttributeID::kEtagValue:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_ETAG_VALUE, value, gerror);
    case DFileInfo::AttributeID::kIdFile:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_ID_FILE, value, gerror);
    case DFileInfo::AttributeID::kIdFilesystem:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_ID_FILESYSTEM, value, gerror);
    case DFileInfo::AttributeID::kMountableUnixDeviceFile:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE, value, gerror);
    case DFileInfo::AttributeID::kMountableHalUdi:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_MOUNTABLE_HAL_UDI, value, gerror);
    case DFileInfo::AttributeID::kOwnerUser:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_OWNER_USER, value, gerror);
    case DFileInfo::AttributeID::kOwnerUserReal:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_OWNER_USER_REAL, value, gerror);
    case DFileInfo::AttributeID::kOwnerGroup:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_OWNER_GROUP, value, gerror);
    case DFileInfo::AttributeID::kFileSystemType:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE, value, gerror);
    case DFileInfo::AttributeID::kGvfsBackend:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_GVFS_BACKEND, value, gerror);
    case DFileInfo::AttributeID::kSelinuxContext:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_SELINUX_CONTEXT, value, gerror);
    case DFileInfo::AttributeID::kTrashDeletionDate:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_TRASH_DELETION_DATE, value, gerror);
    case DFileInfo::AttributeID::kTrashOrigPath:
        return setGFileInfoString(gfile, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH, value, gerror);
    // object
    case DFileInfo::AttributeID::kStandardIcon:
    case DFileInfo::AttributeID::kStandardSymbolicIcon:
    case DFileInfo::AttributeID::kPreviewIcon: {
        // g_file_info_set_attribute_object(gfileinfo, key, value.object());
        //  TODO(lanxs)
        return true;
    }

    default:
        return true;
    }
}

bool DLocalHelper::setAttributeByGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, const QVariant &value)
{
    // discard
    Q_UNUSED(gfileinfo)
    Q_UNUSED(id)
    Q_UNUSED(value)
    return false;
}

std::string DLocalHelper::attributeStringById(DFileInfo::AttributeID id)
{
    if (DLocalHelper::attributeInfoMapFunc().count(id) > 0) {
        const std::string &value = std::get<0>(DLocalHelper::attributeInfoMapFunc().at(id));
        return value;
    }
    return "";
}

QSet<QString> DLocalHelper::hideListFromUrl(const QUrl &url)
{
    g_autofree char *contents = nullptr;
    g_autoptr(GError) error = nullptr;
    gsize len = 0;
    g_autoptr(GFile) hiddenFile = createGFile(url);

    const bool succ = g_file_load_contents(hiddenFile, nullptr, &contents, &len, nullptr, &error);
    if (succ) {
        if (contents && len > 0) {
            QString dataStr(contents);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            const QStringList strs { dataStr.split('\n', Qt::SkipEmptyParts) };
            return QSet<QString> { strs.begin(), strs.end() };
#else
            const QStringList strs { dataStr.split('\n', QString::SkipEmptyParts) };
            return QSet<QString>::fromList(strs);
#endif
        }
    }
    return {};
}

bool DLocalHelper::fileIsHidden(const DFileInfo *dfileinfo, const QSet<QString> &hideList, const bool needRead)
{
    if (!dfileinfo)
        return false;
    // if file not exist DFileInfo::AttributeID::kStandardFileName 返回有错误 bug-200989
    const QString &fileName = dfileinfo->uri().fileName();
    if (fileName.startsWith(".")) {
        return true;
    } else {
        if (hideList.isEmpty() && needRead) {
            const QString &hiddenPath = dfileinfo->attribute(DFileInfo::AttributeID::kStandardParentPath, nullptr).toString() + "/.hidden";
            const QSet<QString> &hideList = DLocalHelper::hideListFromUrl(QUrl::fromLocalFile(hiddenPath));

            if (hideList.contains(fileName))
                return true;
        } else {
            return hideList.contains(fileName);
        }
    }

    return false;
}

bool DLocalHelper::checkGFileType(GFile *file, GFileType type)
{
    if (!file)
        return false;

    g_autoptr(GFileInfo) gfileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, nullptr, nullptr);

    if (!gfileinfo)
        return false;

    return g_file_info_get_file_type(gfileinfo) == type;
}
// fix 多线程排序时，该处的全局变量在compareByString函数中可能导致软件崩溃
// QCollator sortCollator;
class DCollator : public QCollator
{
public:
    DCollator()
        : QCollator()
    {
        setNumericMode(true);
        setCaseSensitivity(Qt::CaseInsensitive);
    }
};

bool DLocalHelper::isNumOrChar(const QChar ch)
{
    QChar normalized;
    if (isFullWidthChar(ch, normalized))
        return isNumOrChar(normalized);

    int chValue = ch.unicode();
    return (chValue >= 48 && chValue <= 57) || (chValue >= 65 && chValue <= 90) || (chValue >= 97 && chValue <= 122);
}

bool DLocalHelper::isNumber(const QChar ch)
{
    QChar number;
    if (isFullWidthChar(ch, number)) {
        return (number.unicode() >= '0' && number.unicode() <= '9');
    }
    return ch.isDigit();
}

bool DLocalHelper::isSymbol(const QChar ch)
{
    // 如果是高代理项，不应该单独判断
    if (ch.isHighSurrogate() || ch.isLowSurrogate())
        return false;

    QChar normalized;
    if (isFullWidthChar(ch, normalized)) {
        return isSymbol(normalized);
    }

    return ch.script() != QChar::Script_Han && !isNumOrChar(ch);
}

bool DLocalHelper::isFullWidthChar(const QChar ch, QChar &normalized)
{
    // 全角字符的 Unicode 范围
    ushort unicode = ch.unicode();

    // 处理全角数字 (0xFF10-0xFF19)
    if (unicode >= 0xFF10 && unicode <= 0xFF19) {
        normalized = QChar(unicode - 0xFF10 + '0');
        return true;
    }

    // 处理全角大写字母 (0xFF21-0xFF3A)
    if (unicode >= 0xFF21 && unicode <= 0xFF3A) {
        normalized = QChar(unicode - 0xFF21 + 'A');
        return true;
    }

    // 处理全角小写字母 (0xFF41-0xFF5A)
    if (unicode >= 0xFF41 && unicode <= 0xFF5A) {
        normalized = QChar(unicode - 0xFF41 + 'a');
        return true;
    }

    // 处理全角标点符号
    static const QHash<ushort, QChar> punctuationMap {
        { 0xFF01, '!' },   // ！
        { 0xFF08, '(' },   // （
        { 0xFF09, ')' },   // ）
        { 0xFF0C, ',' },   // ，
        { 0xFF1A, ':' },   // ：
        { 0xFF1B, ';' },   // ；
        { 0xFF1F, '?' },   // ？
        { 0xFF3B, '[' },   // ［
        { 0xFF3D, ']' },   // ］
        { 0xFF5B, '{' },   // ｛
        { 0xFF5D, '}' },   // ｝
        { 0xFF0E, '.' },   // ．
        { 0xFF0F, '/' },   // ／
        { 0xFF3F, '_' },   // ＿
        { 0xFF0D, '-' },   // －
        { 0xFF1D, '=' },   // ＝
        { 0xFF06, '&' },   // ＆
        { 0xFF5C, '|' },   // ｜
        { 0xFF1C, '<' },   // ＜
        { 0xFF1E, '>' },   // ＞
        { 0xFF02, '"' },   // ＂
        { 0xFF07, '\'' },   // ＇
        { 0xFF0B, '+' },   // ＋
        { 0xFF03, '#' },   // ＃
        { 0xFF04, '$' },   // ＄
        { 0xFF05, '%' },   // ％
        { 0xFF20, '@' },   // ＠
        { 0xFF0A, '*' },   // ＊
        { 0xFF3C, '\\' },   // ＼
        { 0xFF5E, '~' }   // ～
    };

    auto it = punctuationMap.find(unicode);
    if (it != punctuationMap.end()) {
        normalized = it.value();
        return true;
    }

    return false;
}

QString DLocalHelper::numberStr(const QString &str, int pos)
{
    QString tmp;
    int total = str.length();

    // 从当前'pos'开始向前扫描
    while (pos < total) {
        const QChar &ch = str.at(pos);
        if (!isNumber(ch)) {
            break;   // 遇到非数字字符，停止提取
        }

        QChar number;
        // 将全角数字统一转换为半角数字后添加
        if (isFullWidthChar(ch, number)) {
            tmp += number;
        } else {
            tmp += ch;
        }
        pos++;
    }

    return tmp;
}

// The first is smaller than the second and returns true
bool DLocalHelper::compareByStringEx(const QString &str1, const QString &str2)
{
    thread_local static DCollator sortCollator;

    enum CharType {
        NumberType = 0,   // 数字
        LetterType = 1,   // 字母 (非汉字)
        HanType = 2,   // 汉字
        SymbolType = 3   // 符号
    };

    auto getCharType = [](uint unicode) -> CharType {
        // 使用静态 QChar 函数，它们可以安全地处理32位Unicode码点
        if (QChar::isDigit(unicode)) return NumberType;
        // isNumber() 辅助函数可以处理全角数字，但 QChar::isDigit 更高效
        // 我们的 isNumber 仍需在 numberStr 中使用

        QChar::Script script = QChar::script(unicode);
        if (script == QChar::Script_Han) return HanType;
        if (QChar::isLetter(unicode)) return LetterType;
        return SymbolType;
    };

    auto compareUnified = [&](const QString &s1, const QString &s2) -> int {
        QString::const_iterator it1 = s1.constBegin();
        QString::const_iterator it2 = s2.constBegin();

        while (it1 != s1.constEnd() && it2 != s2.constEnd()) {
            int len1 = 1;
            uint unicode1 = it1->unicode();
            if (it1->isHighSurrogate() && (it1 + 1) != s1.constEnd()) {
                unicode1 = QChar::surrogateToUcs4(*it1, *(it1 + 1));
                len1 = 2;
            }

            int len2 = 1;
            uint unicode2 = it2->unicode();
            if (it2->isHighSurrogate() && (it2 + 1) != s2.constEnd()) {
                unicode2 = QChar::surrogateToUcs4(*it2, *(it2 + 1));
                len2 = 2;
            }

            CharType type1 = getCharType(unicode1);
            CharType type2 = getCharType(unicode2);

            if (type1 != type2) {
                return (type1 < type2) ? -1 : 1;
            }

            switch (type1) {
            // ============================ FIX START ============================
            case NumberType: {
                // 提取从当前位置开始的整个数字块
                QString numPart1 = numberStr(s1, it1 - s1.constBegin());
                QString numPart2 = numberStr(s2, it2 - s2.constBegin());

                // 规则1: 使用QCollator按数值大小比较
                int numCompareResult = sortCollator.compare(numPart1, numPart2);
                if (numCompareResult != 0) {
                    return numCompareResult;
                }

                // 规则2: 数值相同，比较原始长度 (前导零多的排前面)
                // numberStr返回的是规范化后的半角字符串，其长度可能与原始长度不同。
                // 我们需要计算原始字符串中数字块的真实长度。
                int rawLen1 = 0;
                for (auto temp_it = it1; temp_it != s1.constEnd() && isNumber(*temp_it); ++temp_it)
                    rawLen1++;
                int rawLen2 = 0;
                for (auto temp_it = it2; temp_it != s2.constEnd() && isNumber(*temp_it); ++temp_it)
                    rawLen2++;

                if (rawLen1 != rawLen2) {
                    // 原始字符串更长的排在前面
                    return (rawLen1 > rawLen2) ? -1 : 1;
                }

                // 数值和原始长度都相同，跳过这个块继续比较
                it1 += rawLen1;
                it2 += rawLen2;
                continue;
            }
            // ============================= FIX END =============================
            case LetterType:
            case HanType: {
                // 使用 char32_t* overload to avoid deprecation warning.
                QString charStr1 = QString::fromUcs4(reinterpret_cast<const char32_t *>(&unicode1), 1);
                QString charStr2 = QString::fromUcs4(reinterpret_cast<const char32_t *>(&unicode2), 1);

                int result = sortCollator.compare(charStr1, charStr2);
                if (result != 0) return result;

                // 规则3平局决胜: a A b B
                if (QChar::isLetter(unicode1) && QChar::isLetter(unicode2)) {
                    if (QChar::toLower(unicode1) == QChar::toLower(unicode2)) {
                        // 如果小写形式相同，小写字母排在前面
                        if (QChar::isLower(unicode1) != QChar::isLower(unicode2)) {
                            return QChar::isLower(unicode1) ? -1 : 1;
                        }
                    }
                }
                break;
            }
            case SymbolType: {
                if (unicode1 != unicode2) {
                    return (unicode1 < unicode2) ? -1 : 1;
                }
                break;
            }
            }
            it1 += len1;
            it2 += len2;
        }

        if (it1 == s1.constEnd() && it2 != s2.constEnd()) return -1;
        if (it1 != s1.constEnd() && it2 == s2.constEnd()) return 1;
        return 0;
    };

    return compareUnified(str1, str2) < 0;
}

bool DLocalHelper::compareByString(const QString &str1, const QString &str2)
{
    // 处理文件名称为  新建文件a 新建文件夹 排序错误的问题
    //  按名称排序
    //  1、按名称排序规则为：数字→字母→汉字→其它；
    //  2、其中数字由小到大排列，字母由a～z、 A~Z排列（例如：a A b B），汉字:按拼音首字母由a～z排列；
    //  3、如果首字母相同看第二位字母，以此类推；
    //  4、其他：特殊字符和乱码排在后面；
    return compareByStringEx(str1, str2);
}

int DLocalHelper::compareByName(const FTSENT **left, const FTSENT **right)
{
    QString str1 = QString((*left)->fts_name), str2 = QString((*right)->fts_name);
    auto tt = compareByString(str1, str2);
    return tt ? -1 : 1;
}

int DLocalHelper::compareBySize(const FTSENT **left, const FTSENT **right)
{
    auto leftSize = fileSizeByEnt(left);
    auto rightSize = fileSizeByEnt(right);
    if (leftSize == rightSize)
        return compareByName(left, right);
    return leftSize > rightSize;
}

int DLocalHelper::compareByLastModifed(const FTSENT **left, const FTSENT **right)
{
    if ((*left)->fts_statp->st_mtim.tv_sec == (*right)->fts_statp->st_mtim.tv_sec) {
        if ((*left)->fts_statp->st_mtim.tv_nsec > (*right)->fts_statp->st_mtim.tv_nsec)
            return compareByName(left, right);
        return (*left)->fts_statp->st_mtim.tv_nsec > (*right)->fts_statp->st_mtim.tv_nsec;
    }
    return (*left)->fts_statp->st_mtim.tv_sec > (*right)->fts_statp->st_mtim.tv_sec;
}

int DLocalHelper::compareByLastRead(const FTSENT **left, const FTSENT **right)
{
    if ((*left)->fts_statp->st_atim.tv_sec == (*right)->fts_statp->st_atim.tv_sec) {
        if ((*left)->fts_statp->st_atim.tv_nsec > (*right)->fts_statp->st_atim.tv_nsec)
            return compareByName(left, right);
        return (*left)->fts_statp->st_atim.tv_nsec > (*right)->fts_statp->st_atim.tv_nsec;
    }
    return (*left)->fts_statp->st_atim.tv_sec > (*right)->fts_statp->st_atim.tv_sec;
}

QSharedPointer<DEnumerator::SortFileInfo> DLocalHelper::createSortFileInfo(const FTSENT *ent,
                                                                           const QSet<QString> hidList)
{
    auto sortPointer = QSharedPointer<DEnumerator::SortFileInfo>(new DEnumerator::SortFileInfo);
    auto name = QString(ent->fts_name);
    sortPointer->filesize = ent->fts_statp->st_size;
    sortPointer->isSymLink = S_ISLNK(ent->fts_statp->st_mode);
    sortPointer->isDir = S_ISDIR(ent->fts_statp->st_mode);
    if (sortPointer->isSymLink) {
        QString symlinkTagetPath = resolveSymlink(QUrl::fromLocalFile(ent->fts_path));
        if (!symlinkTagetPath.isEmpty())
            sortPointer->symlinkUrl = QUrl::fromLocalFile(symlinkTagetPath);
    }


    if (sortPointer->symlinkUrl.isValid() && !DFMUtils::isGvfsFile(sortPointer->symlinkUrl)) {
        struct stat st;
        if (stat(sortPointer->symlinkUrl.path().toUtf8().data(), &st) == 0) {
            sortPointer->filesize = st.st_size;
            sortPointer->isDir = S_ISDIR(st.st_mode);
        }
    }

    sortPointer->isFile = !sortPointer->isDir;
    sortPointer->isHide = name.startsWith(".") ? true : hidList.contains(name);
    sortPointer->isReadable = ent->fts_statp->st_mode & S_IREAD;
    sortPointer->isWriteable = ent->fts_statp->st_mode & S_IWRITE;
    sortPointer->isExecutable = ent->fts_statp->st_mode & S_IEXEC;
    sortPointer->url = QUrl::fromLocalFile(ent->fts_path);
    if (DFMUtils::isInvalidCodecByPath(ent->fts_path))
        sortPointer->url.setUserInfo("originPath::" + QString::fromLatin1(ent->fts_path));
    sortPointer->inode = ent->fts_statp->st_ino;
    sortPointer->gid = ent->fts_statp->st_gid;
    sortPointer->uid = ent->fts_statp->st_uid;
    sortPointer->lastRead = ent->fts_statp->st_atim.tv_sec;
    sortPointer->lastReadNs = ent->fts_statp->st_atim.tv_nsec;
    sortPointer->lastModifed = ent->fts_statp->st_mtim.tv_sec;
    sortPointer->lastModifedNs = ent->fts_statp->st_mtim.tv_nsec;
    sortPointer->create = ent->fts_statp->st_ctim.tv_sec;
    sortPointer->createNs = ent->fts_statp->st_ctim.tv_nsec;

    return sortPointer;
}

GFile *DLocalHelper::createGFile(const QUrl &uri)
{
    QString path = uri.userInfo().isEmpty() || !uri.userInfo().startsWith("originPath::") ?
                QString() : uri.userInfo().replace("originPath::", "");

    GFile *gfile = path.isEmpty() ?
                g_file_new_for_uri(uri.toString().toLocal8Bit().data()) :
                g_file_new_for_path(path.toLatin1().data());
    return gfile;
}

QVariant DLocalHelper::getGFileInfoIcon(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (!g_file_info_has_attribute(gfileinfo, key)) {
        errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
        return QVariant();
    }
    GObject *icon = g_file_info_get_attribute_object(gfileinfo, key);
    if (!icon)
        return QVariant();

    QList<QString> ret;
    auto names = g_themed_icon_get_names(G_THEMED_ICON(icon));
    for (int j = 0; names && names[j] != nullptr; ++j) {
        if (strcmp(names[j], "folder") == 0)
            ret.prepend(QString::fromLocal8Bit(names[j]));
        else
            ret.append(QString::fromLocal8Bit(names[j]));
    }

    return QVariant(ret);
}

QVariant DLocalHelper::getGFileInfoString(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (g_file_info_has_attribute(gfileinfo, key))
        return g_file_info_get_attribute_string(gfileinfo, key);
    errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
    return QVariant();
}

QVariant DLocalHelper::getGFileInfoByteString(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (g_file_info_has_attribute(gfileinfo, key))
        return g_file_info_get_attribute_byte_string(gfileinfo, key);
    errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
    return QVariant();
}

QVariant DLocalHelper::getGFileInfoBool(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (g_file_info_has_attribute(gfileinfo, key))
        return g_file_info_get_attribute_boolean(gfileinfo, key);
    errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
    return QVariant();
}

QVariant DLocalHelper::getGFileInfoUint32(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (g_file_info_has_attribute(gfileinfo, key))
        return g_file_info_get_attribute_uint32(gfileinfo, key);
    errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
    return QVariant();
}

QVariant DLocalHelper::getGFileInfoInt32(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (g_file_info_has_attribute(gfileinfo, key))
        return g_file_info_get_attribute_int32(gfileinfo, key);
    errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
    return QVariant();
}

QVariant DLocalHelper::getGFileInfoUint64(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (g_file_info_has_attribute(gfileinfo, key))
        return qulonglong(g_file_info_get_attribute_uint64(gfileinfo, key));
    errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
    return QVariant();
}

QVariant DLocalHelper::getGFileInfoInt64(GFileInfo *gfileinfo, const char *key, DFMIOErrorCode &errorcode)
{
    assert(key != nullptr);
    if (g_file_info_has_attribute(gfileinfo, key))
        return qlonglong(g_file_info_get_attribute_int64(gfileinfo, key));
    errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
    return QVariant();
}

bool DLocalHelper::setGFileInfoString(GFile *gfile, const char *key, const QVariant &value, GError **gerror)
{
    assert(key != nullptr);
    g_file_set_attribute_string(gfile, key, value.toString().toLocal8Bit().data(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
    if (gerror) {
        g_autofree gchar *url = g_file_get_uri(gfile);
        qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
        return false;
    }
    return true;
}

bool DLocalHelper::setGFileInfoByteString(GFile *gfile, const char *key, const QVariant &value, GError **gerror)
{
    assert(key != nullptr);
    g_file_set_attribute_byte_string(gfile, key, value.toString().toLocal8Bit().data(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
    if (gerror) {
        g_autofree gchar *url = g_file_get_uri(gfile);
        qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
        return false;
    }
    return true;
}

bool DLocalHelper::setGFileInfoBool(GFile *gfile, const char *key, const QVariant &value, GError **gerror)
{
    assert(key != nullptr);
    gboolean b = value.toBool();
    gpointer gpValue = &b;
    g_file_set_attribute(gfile, key, G_FILE_ATTRIBUTE_TYPE_BOOLEAN, gpValue, G_FILE_QUERY_INFO_NONE, nullptr, gerror);
    if (gerror) {
        g_autofree gchar *url = g_file_get_uri(gfile);
        qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
        return false;
    }
    return true;
}

bool DLocalHelper::setGFileInfoUint32(GFile *gfile, const char *key, const QVariant &value, GError **gerror)
{
    assert(key != nullptr);
    g_file_set_attribute_uint32(gfile, key, value.toUInt(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
    if (gerror) {
        g_autofree gchar *url = g_file_get_uri(gfile);
        qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
        return false;
    }
    return true;
}

bool DLocalHelper::setGFileInfoInt32(GFile *gfile, const char *key, const QVariant &value, GError **gerror)
{
    assert(key != nullptr);
    g_file_set_attribute_int32(gfile, key, value.toInt(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
    if (gerror) {
        g_autofree gchar *url = g_file_get_uri(gfile);
        qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
        return false;
    }
    return true;
}

bool DLocalHelper::setGFileInfoUint64(GFile *gfile, const char *key, const QVariant &value, GError **gerror)
{
    assert(key != nullptr);
    g_file_set_attribute_uint64(gfile, key, value.toULongLong(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
    if (gerror) {
        g_autofree gchar *url = g_file_get_uri(gfile);
        qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
        return false;
    }
    return true;
}

bool DLocalHelper::setGFileInfoInt64(GFile *gfile, const char *key, const QVariant &value, GError **gerror)
{
    assert(key != nullptr);
    g_file_set_attribute_int64(gfile, key, value.toLongLong(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
    if (gerror) {
        g_autofree gchar *url = g_file_get_uri(gfile);
        qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
        return false;
    }
    return true;
}

QString DLocalHelper::symlinkTarget(const QUrl &url)
{
    char buffer[4096] { 0 };
    auto size = readlink(url.path().toStdString().c_str(), buffer, sizeof(buffer));
    if (size > 0)
        return QString::fromUtf8(buffer, static_cast<int>(size));
    return QString();
}

QString DLocalHelper::resolveSymlink(const QUrl &url)
{
    QSet<QString> visited;
    QString target = DLocalHelper::symlinkTarget(url);
    while (!target.isEmpty()) {
        if (visited.contains(target))
            return QString();   // Cycle detected: return empty
        visited.insert(target);
        QUrl newUrl = QUrl::fromLocalFile(target);
        QString nextTarget = DLocalHelper::symlinkTarget(newUrl);
        if (nextTarget.isEmpty())
            break;
        target = nextTarget;
    }
    return target;
}

qint64 DLocalHelper::fileSizeByEnt(const FTSENT **ent)
{
    if (!S_ISLNK((*ent)->fts_statp->st_mode))
        return S_ISDIR((*ent)->fts_statp->st_mode) ? -1 : (*ent)->fts_statp->st_size;

    // 这里很奇怪 fts_name="tt 快捷方式" fts_path="/home/uos/Desktop" 所以重新拼接"/home/ut005319@uos/Desktop/tt 快捷方式"
    auto filePath = QString((*ent)->fts_path) + QDir::separator() + (*ent)->fts_name;
    QString linkTag = resolveSymlink(QUrl::fromLocalFile(filePath));
    struct stat st;
    qint64 size = (*ent)->fts_statp->st_size;
    if (linkTag.isEmpty() || DFMUtils::isGvfsFile(QUrl::fromLocalFile(linkTag)) || stat(linkTag.toUtf8().data(), &st) != 0)
        size = (*ent)->fts_statp->st_size;
    else {
        size = S_ISDIR(st.st_mode) ? -1 : st.st_size;
    }
    return size;
}
