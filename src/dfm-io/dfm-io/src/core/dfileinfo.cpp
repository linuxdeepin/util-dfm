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

#include "core/dfileinfo_p.h"

USING_IO_NAMESPACE

DFileInfo::AttributeInfoMap DFileInfo::attributeInfoMap = {
    { DFileInfo::AttributeID::StandardType, std::make_tuple<std::string, QVariant>("standard::type", 0) },   // G_FILE_ATTRIBUTE_STANDARD_TYPE
    { DFileInfo::AttributeID::StandardIsHidden, std::make_tuple<std::string, QVariant>("standard::is-hidden", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN
    { DFileInfo::AttributeID::StandardIsBackup, std::make_tuple<std::string, QVariant>("standard::is-backup", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP
    { DFileInfo::AttributeID::StandardIsSymlink, std::make_tuple<std::string, QVariant>("standard::is-symlink", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK
    { DFileInfo::AttributeID::StandardIsVirtual, std::make_tuple<std::string, QVariant>("standard::is-virtual", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL
    { DFileInfo::AttributeID::StandardIsVolatile, std::make_tuple<std::string, QVariant>("standard::is-volatile", false) },   // G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE
    { DFileInfo::AttributeID::StandardName, std::make_tuple<std::string, QVariant>("standard::name", "") },   // G_FILE_ATTRIBUTE_STANDARD_NAME
    { DFileInfo::AttributeID::StandardDisplayName, std::make_tuple<std::string, QVariant>("standard::display-name", "") },   // G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME
    { DFileInfo::AttributeID::StandardEditName, std::make_tuple<std::string, QVariant>("standard::edit-name", "") },   // G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME
    { DFileInfo::AttributeID::StandardCopyName, std::make_tuple<std::string, QVariant>("standard::copy-name", "") },   // G_FILE_ATTRIBUTE_STANDARD_COPY_NAME
    { DFileInfo::AttributeID::StandardIcon, std::make_tuple<std::string, QVariant>("standard::icon", 0) },   // G_FILE_ATTRIBUTE_STANDARD_ICON
    { DFileInfo::AttributeID::StandardSymbolicIcon, std::make_tuple<std::string, QVariant>("standard::symbolic-icon", 0) },   // G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON
    { DFileInfo::AttributeID::StandardContentType, std::make_tuple<std::string, QVariant>("standard::content-type", "") },   // G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE
    { DFileInfo::AttributeID::StandardFastContentType, std::make_tuple<std::string, QVariant>("standard::fast-content-type", "") },   // G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE
    { DFileInfo::AttributeID::StandardSize, std::make_tuple<std::string, QVariant>("standard::size", 0) },   // G_FILE_ATTRIBUTE_STANDARD_SIZE
    { DFileInfo::AttributeID::StandardAllocatedSize, std::make_tuple<std::string, QVariant>("standard::allocated-size", 0) },   // G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE
    { DFileInfo::AttributeID::StandardSymlinkTarget, std::make_tuple<std::string, QVariant>("standard::symlink-target", "") },   // G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET
    { DFileInfo::AttributeID::StandardTargetUri, std::make_tuple<std::string, QVariant>("standard::target-uri", "") },   // G_FILE_ATTRIBUTE_STANDARD_TARGET_URI
    { DFileInfo::AttributeID::StandardSortOrder, std::make_tuple<std::string, QVariant>("standard::sort-order", 0) },   // G_FILE_ATTRIBUTE_STANDARD_SORT_ORDER
    { DFileInfo::AttributeID::StandardDescription, std::make_tuple<std::string, QVariant>("standard::description", "") },   // G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION

    { DFileInfo::AttributeID::EtagValue, std::make_tuple<std::string, QVariant>("etag::value", "") },   // G_FILE_ATTRIBUTE_ETAG_VALUE

    { DFileInfo::AttributeID::IdFile, std::make_tuple<std::string, QVariant>("id::file", "") },   // G_FILE_ATTRIBUTE_ID_FILE
    { DFileInfo::AttributeID::IdFilesystem, std::make_tuple<std::string, QVariant>("id::filesystem", "") },   // G_FILE_ATTRIBUTE_ID_FILESYSTEM

    { DFileInfo::AttributeID::AccessCanRead, std::make_tuple<std::string, QVariant>("access::can-read", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_READ
    { DFileInfo::AttributeID::AccessCanWrite, std::make_tuple<std::string, QVariant>("access::can-write", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE
    { DFileInfo::AttributeID::AccessCanExecute, std::make_tuple<std::string, QVariant>("access::can-execute", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE
    { DFileInfo::AttributeID::AccessCanDelete, std::make_tuple<std::string, QVariant>("access::can-delete", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE
    { DFileInfo::AttributeID::AccessCanTrash, std::make_tuple<std::string, QVariant>("access::can-trash", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH
    { DFileInfo::AttributeID::AccessCanRename, std::make_tuple<std::string, QVariant>("access::can-rename", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME

    { DFileInfo::AttributeID::MountableCanMount, std::make_tuple<std::string, QVariant>("mountable::can-mount", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT
    { DFileInfo::AttributeID::MountableCanUnmount, std::make_tuple<std::string, QVariant>("mountable::can-unmount", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT
    { DFileInfo::AttributeID::MountableCanEject, std::make_tuple<std::string, QVariant>("mountable::can-eject", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT
    { DFileInfo::AttributeID::MountableUnixDevice, std::make_tuple<std::string, QVariant>("mountable::unix-device", 0) },   // G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE
    { DFileInfo::AttributeID::MountableUnixDeviceFile, std::make_tuple<std::string, QVariant>("mountable::unix-device-file", "") },   // G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE
    { DFileInfo::AttributeID::MountableHalUdi, std::make_tuple<std::string, QVariant>("mountable::hal-udi", "") },   // G_FILE_ATTRIBUTE_MOUNTABLE_HAL_UDI
    { DFileInfo::AttributeID::MountableCanPoll, std::make_tuple<std::string, QVariant>("mountable::can-poll", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_POLL
    { DFileInfo::AttributeID::MountableIsMediaCheckAutomatic, std::make_tuple<std::string, QVariant>("mountable::is-media-check-automatic", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_IS_MEDIA_CHECK_AUTOMATIC
    { DFileInfo::AttributeID::MountableCanStart, std::make_tuple<std::string, QVariant>("mountable::can-start", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START
    { DFileInfo::AttributeID::MountableCanStartDegraded, std::make_tuple<std::string, QVariant>("mountable::can-start-degraded", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START_DEGRADED
    { DFileInfo::AttributeID::MountableCanStop, std::make_tuple<std::string, QVariant>("mountable::can-stop", false) },   // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_STOP
    { DFileInfo::AttributeID::MountableStartStopType, std::make_tuple<std::string, QVariant>("mountable::start-stop-type", 0) },   // G_FILE_ATTRIBUTE_MOUNTABLE_START_STOP_TYPE

    { DFileInfo::AttributeID::TimeModified, std::make_tuple<std::string, QVariant>("time::modified", 0) },   // G_FILE_ATTRIBUTE_TIME_MODIFIED
    { DFileInfo::AttributeID::TimeModifiedUsec, std::make_tuple<std::string, QVariant>("time::modified-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC
    { DFileInfo::AttributeID::TimeAccess, std::make_tuple<std::string, QVariant>("time::access", 0) },   // G_FILE_ATTRIBUTE_TIME_ACCESS
    { DFileInfo::AttributeID::TimeAccessUsec, std::make_tuple<std::string, QVariant>("time::access-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_ACCESS_USEC
    { DFileInfo::AttributeID::TimeChanged, std::make_tuple<std::string, QVariant>("time::changed", 0) },   // G_FILE_ATTRIBUTE_TIME_CHANGED
    { DFileInfo::AttributeID::TimeChangedUsec, std::make_tuple<std::string, QVariant>("time::changed-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_CHANGED_USEC
    { DFileInfo::AttributeID::TimeCreated, std::make_tuple<std::string, QVariant>("time::created", 0) },   // G_FILE_ATTRIBUTE_TIME_CREATED
    { DFileInfo::AttributeID::TimeCreatedUsec, std::make_tuple<std::string, QVariant>("time::created-usec", 0) },   // G_FILE_ATTRIBUTE_TIME_CREATED_USEC

    { DFileInfo::AttributeID::UnixDevice, std::make_tuple<std::string, QVariant>("unix::device", 0) },   // G_FILE_ATTRIBUTE_UNIX_DEVICE
    { DFileInfo::AttributeID::UnixInode, std::make_tuple<std::string, QVariant>("unix::inode", 0) },   // G_FILE_ATTRIBUTE_UNIX_INODE
    { DFileInfo::AttributeID::UnixMode, std::make_tuple<std::string, QVariant>("unix::mode", 0) },   // G_FILE_ATTRIBUTE_UNIX_MODE
    { DFileInfo::AttributeID::UnixNlink, std::make_tuple<std::string, QVariant>("unix::nlink", 0) },   // G_FILE_ATTRIBUTE_UNIX_NLINK
    { DFileInfo::AttributeID::UnixUID, std::make_tuple<std::string, QVariant>("unix::uid", 0) },   // G_FILE_ATTRIBUTE_UNIX_UID
    { DFileInfo::AttributeID::UnixGID, std::make_tuple<std::string, QVariant>("unix::gid", 0) },   // G_FILE_ATTRIBUTE_UNIX_GID
    { DFileInfo::AttributeID::UnixRdev, std::make_tuple<std::string, QVariant>("unix::rdev", 0) },   // G_FILE_ATTRIBUTE_UNIX_RDEV
    { DFileInfo::AttributeID::UnixBlockSize, std::make_tuple<std::string, QVariant>("unix::block-size", 0) },   // G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE
    { DFileInfo::AttributeID::UnixBlocks, std::make_tuple<std::string, QVariant>("unix::blocks", 0) },   // G_FILE_ATTRIBUTE_UNIX_BLOCKS
    { DFileInfo::AttributeID::UnixIsMountPoint, std::make_tuple<std::string, QVariant>("unix::is-mountpoint", false) },   // G_FILE_ATTRIBUTE_UNIX_IS_MOUNTPOINT

    { DFileInfo::AttributeID::DosIsArchive, std::make_tuple<std::string, QVariant>("dos::is-archive", false) },   // G_FILE_ATTRIBUTE_DOS_IS_ARCHIVE
    { DFileInfo::AttributeID::DosIsSystem, std::make_tuple<std::string, QVariant>("dos::is-system", false) },   // G_FILE_ATTRIBUTE_DOS_IS_SYSTEM

    { DFileInfo::AttributeID::OwnerUser, std::make_tuple<std::string, QVariant>("owner::user", "") },   // G_FILE_ATTRIBUTE_OWNER_USER
    { DFileInfo::AttributeID::OwnerUserReal, std::make_tuple<std::string, QVariant>("owner::user-real", "") },   // G_FILE_ATTRIBUTE_OWNER_USER_REAL
    { DFileInfo::AttributeID::OwnerGroup, std::make_tuple<std::string, QVariant>("owner::group", "") },   // G_FILE_ATTRIBUTE_OWNER_GROUP

    { DFileInfo::AttributeID::ThumbnailPath, std::make_tuple<std::string, QVariant>("thumbnail::path", "") },   // G_FILE_ATTRIBUTE_THUMBNAIL_PATH
    { DFileInfo::AttributeID::ThumbnailFailed, std::make_tuple<std::string, QVariant>("thumbnail::failed", false) },   // G_FILE_ATTRIBUTE_THUMBNAILING_FAILED
    { DFileInfo::AttributeID::ThumbnailIsValid, std::make_tuple<std::string, QVariant>("thumbnail::is-valid", false) },   // G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID

    { DFileInfo::AttributeID::PreviewIcon, std::make_tuple<std::string, QVariant>("preview::icon", 0) },   // G_FILE_ATTRIBUTE_PREVIEW_ICON

    { DFileInfo::AttributeID::FileSystemSize, std::make_tuple<std::string, QVariant>("filesystem::size", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_SIZE
    { DFileInfo::AttributeID::FileSystemFree, std::make_tuple<std::string, QVariant>("filesystem::free", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_FREE
    { DFileInfo::AttributeID::FileSystemUsed, std::make_tuple<std::string, QVariant>("filesystem::used", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_USED
    { DFileInfo::AttributeID::FileSystemType, std::make_tuple<std::string, QVariant>("filesystem::type", "") },   // G_FILE_ATTRIBUTE_FILESYSTEM_TYPE
    { DFileInfo::AttributeID::FileSystemReadOnly, std::make_tuple<std::string, QVariant>("filesystem::readonly", false) },   // G_FILE_ATTRIBUTE_FILESYSTEM_READONLY
    { DFileInfo::AttributeID::FileSystemUsePreview, std::make_tuple<std::string, QVariant>("filesystem::use-preview", 0) },   // G_FILE_ATTRIBUTE_FILESYSTEM_USE_PREVIEW
    { DFileInfo::AttributeID::FileSystemRemote, std::make_tuple<std::string, QVariant>("filesystem::remote", false) },   // G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE

    { DFileInfo::AttributeID::GvfsBackend, std::make_tuple<std::string, QVariant>("gvfs::backend", "") },   // G_FILE_ATTRIBUTE_GVFS_BACKEND

    { DFileInfo::AttributeID::SelinuxContext, std::make_tuple<std::string, QVariant>("selinux::context", "") },   // G_FILE_ATTRIBUTE_SELINUX_CONTEXT

    { DFileInfo::AttributeID::TrashItemCount, std::make_tuple<std::string, QVariant>("trash::item-count", 0) },   // G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT
    { DFileInfo::AttributeID::TrashDeletionDate, std::make_tuple<std::string, QVariant>("trash::deletion-date", "") },   // G_FILE_ATTRIBUTE_TRASH_DELETION_DATE
    { DFileInfo::AttributeID::TrashOrigPath, std::make_tuple<std::string, QVariant>("trash::orig-path", "") },   // G_FILE_ATTRIBUTE_TRASH_ORIG_PATH

    { DFileInfo::AttributeID::RecentModified, std::make_tuple<std::string, QVariant>("recent::modified", 0) },   // G_FILE_ATTRIBUTE_RECENT_MODIFIED
    { DFileInfo::AttributeID::ExtendWordSize, std::make_tuple<std::string, QVariant>("xattr::word-size", 0) },   // user extend attr
    { DFileInfo::AttributeID::ExtendMediaDuration, std::make_tuple<std::string, QVariant>("xattr::media-duration", 0) },   // user extend attr

    { DFileInfo::AttributeID::CustomStart, std::make_tuple<std::string, QVariant>("custom-start", 0) },

    { DFileInfo::AttributeID::StandardIsFile, std::make_tuple<std::string, QVariant>("standard::is-file", false) },
    { DFileInfo::AttributeID::StandardIsDir, std::make_tuple<std::string, QVariant>("standard::is-dir", false) },
    { DFileInfo::AttributeID::StandardIsRoot, std::make_tuple<std::string, QVariant>("standard::is-root", false) },
    { DFileInfo::AttributeID::StandardSuffix, std::make_tuple<std::string, QVariant>("standard::suffix", "") },
    { DFileInfo::AttributeID::StandardCompleteSuffix, std::make_tuple<std::string, QVariant>("standard::complete-suffix", "") },
    { DFileInfo::AttributeID::StandardFilePath, std::make_tuple<std::string, QVariant>("standard::file-path", "") },
    { DFileInfo::AttributeID::StandardParentPath, std::make_tuple<std::string, QVariant>("standard::parent-path", "") },
    { DFileInfo::AttributeID::StandardBaseName, std::make_tuple<std::string, QVariant>("standard::base-name", "") },
    { DFileInfo::AttributeID::StandardFileName, std::make_tuple<std::string, QVariant>("standard::file-name", "") },
    { DFileInfo::AttributeID::StandardCompleteBaseName, std::make_tuple<std::string, QVariant>("standard::complete-base-name", "") },
};

DFileInfo::DFileInfo()
    : d(new DFileInfoPrivate(this))
{
}

DFileInfo::DFileInfo(const QUrl &uri)
    : d(new DFileInfoPrivate(this))
{
    d->uri = uri;
}

DFileInfo::DFileInfo(const DFileInfo &info)
    : d(info.d)
{
}

DFileInfo::~DFileInfo()
{
}

DFileInfo &DFileInfo::operator=(const DFileInfo &info)
{
    d = info.d;
    return *this;
}

QVariant DFileInfo::attribute(DFileInfo::AttributeID id, bool *success) const
{
    if (d->attributeFunc)
        return d->attributeFunc(id, success);

    if (success)
        *success = false;
    return std::get<1>(DFileInfo::attributeInfoMap.at(id));
}

bool DFileInfo::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    if (!d->setAttributeFunc)
        return false;

    return d->setAttributeFunc(id, value);
}

bool DFileInfo::hasAttribute(DFileInfo::AttributeID id) const
{
    if (!d->hasAttributeFunc)
        return false;

    return d->hasAttributeFunc(id);
}

bool DFileInfo::removeAttribute(DFileInfo::AttributeID id)
{
    if (!d->removeAttributeFunc)
        return false;

    return d->removeAttributeFunc(id);
}

QList<DFileInfo::AttributeID> DFileInfo::attributeIDList() const
{
    if (!d->attributeListFunc)
        return QList<DFileInfo::AttributeID>();

    return d->attributeListFunc();
}

bool DFileInfo::exists() const
{
    if (!d)
        return false;
    if (!d->existsFunc)
        return false;
    return d->existsFunc();
}

/*!
 * @brief flush attribute by @setAttribute to disk
 * @param
 * @return
 */
bool DFileInfo::flush()
{
    if (d->flushFunc)
        return d->flushFunc();
    return false;
}

DFile::Permissions DFileInfo::permissions()
{
    if (d->permissionFunc)
        return d->permissionFunc();
    return DFile::Permission::NoPermission;
}

void DFileInfo::registerAttribute(const DFileInfo::AttributeFunc &func)
{
    d->attributeFunc = func;
}

void DFileInfo::registerSetAttribute(const DFileInfo::SetAttributeFunc &func)
{
    d->setAttributeFunc = func;
}

void DFileInfo::registerHasAttribute(const DFileInfo::HasAttributeFunc &func)
{
    d->hasAttributeFunc = func;
}

void DFileInfo::registerRemoveAttribute(const DFileInfo::RemoveAttributeFunc &func)
{
    d->removeAttributeFunc = func;
}

void DFileInfo::registerAttributeList(const DFileInfo::AttributeListFunc &func)
{
    d->attributeListFunc = func;
}

void DFileInfo::registerExists(const DFileInfo::ExistsFunc &func)
{
    d->existsFunc = func;
}

void DFileInfo::registerFlush(const DFileInfo::FlushFunc &func)
{
    d->flushFunc = func;
}

void DFileInfo::registerPermissions(const DFile::PermissionFunc &func)
{
    d->permissionFunc = func;
}

void DFileInfo::registerLastError(const DFileInfo::LastErrorFunc &func)
{
    d->lastErrorFunc = func;
}

QUrl DFileInfo::uri() const
{
    return d->uri;
}

QString DFileInfo::dump() const
{
    QString ret;
    for (const auto &[id, key] : attributeInfoMap) {
        const QVariant &&value = attribute(id);
        if (value.isValid()) {
            ret.append(std::get<0>(attributeInfoMap.at(id)).c_str());
            ret.append(":");
            ret.append(value.toString());
            ret.append("\n");
        }
    }
    return ret;
}

DFMIOError DFileInfo::lastError() const
{
    if (!d->lastErrorFunc)
        return DFMIOError();

    return d->lastErrorFunc();
}
