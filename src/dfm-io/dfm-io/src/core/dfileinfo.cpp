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

DFileInfo::AttributeNameMap DFileInfo::attributeNames = {
    {DFileInfo::AttributeID::StandardType, "standard::type"}, // G_FILE_ATTRIBUTE_STANDARD_TYPE
    {DFileInfo::AttributeID::StandardIsHidden, "standard::is-hidden"}, // G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN
    {DFileInfo::AttributeID::StandardIsBackup, "standard::is-backup"}, // G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP
    {DFileInfo::AttributeID::StandardIsSymlink, "standard::is-symlink"}, // G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK
    {DFileInfo::AttributeID::StandardIsVirtual, "standard::is-virtual"}, // G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL
    {DFileInfo::AttributeID::StandardIsVolatile, "standard::is-volatile"}, // G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE
    {DFileInfo::AttributeID::StandardName, "standard::name"}, // G_FILE_ATTRIBUTE_STANDARD_NAME
    {DFileInfo::AttributeID::StandardDisplayName, "standard::display-name"}, // G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME
    {DFileInfo::AttributeID::StandardEditName, "standard::edit-name"}, // G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME
    {DFileInfo::AttributeID::StandardCopyName, "standard::copy-name"}, // G_FILE_ATTRIBUTE_STANDARD_COPY_NAME
    {DFileInfo::AttributeID::StandardIcon, "standard::icon"}, // G_FILE_ATTRIBUTE_STANDARD_ICON
    {DFileInfo::AttributeID::StandardSymbolicIcon, "standard::symbolic-icon"}, // G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON
    {DFileInfo::AttributeID::StandardContentType, "standard::content-type"}, // G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE
    {DFileInfo::AttributeID::StandardFastContentType, "standard::fast-content-type"}, // G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE
    {DFileInfo::AttributeID::StandardSize, "standard::size"}, // G_FILE_ATTRIBUTE_STANDARD_SIZE
    {DFileInfo::AttributeID::StandardAllocatedSize, "standard::allocated-size"}, // G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE
    {DFileInfo::AttributeID::StandardSymlinkTarget, "standard::symlink-target"}, // G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET
    {DFileInfo::AttributeID::StandardTargetUri, "standard::target-uri"}, // G_FILE_ATTRIBUTE_STANDARD_TARGET_URI
    {DFileInfo::AttributeID::StandardSortOrder, "standard::sort-order"}, // G_FILE_ATTRIBUTE_STANDARD_SORT_ORDER
    {DFileInfo::AttributeID::StandardDescription, "standard::description"}, // G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION

    {DFileInfo::AttributeID::EtagValue, "etag::value"}, // G_FILE_ATTRIBUTE_ETAG_VALUE

    {DFileInfo::AttributeID::IdFile, "id::file"}, // G_FILE_ATTRIBUTE_ID_FILE
    {DFileInfo::AttributeID::IdFilesystem, "id::filesystem"}, // G_FILE_ATTRIBUTE_ID_FILESYSTEM

    {DFileInfo::AttributeID::AccessCanRead, "access::can-read"}, // G_FILE_ATTRIBUTE_ACCESS_CAN_READ
    {DFileInfo::AttributeID::AccessCanWrite, "access::can-write"}, // G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE
    {DFileInfo::AttributeID::AccessCanExecute, "access::can-execute"}, // G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE
    {DFileInfo::AttributeID::AccessCanDelete, "access::can-delete"}, // G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE
    {DFileInfo::AttributeID::AccessCanTrash, "access::can-trash"}, // G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH
    {DFileInfo::AttributeID::AccessCanRename, "access::can-rename"}, // G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME

    {DFileInfo::AttributeID::MountableCanMount, "mountable::can-mount"}, // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT
    {DFileInfo::AttributeID::MountableCanUnmount, "mountable::can-unmount"}, // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT
    {DFileInfo::AttributeID::MountableCanEject, "mountable::can-eject"}, // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT
    {DFileInfo::AttributeID::MountableUnixDevice, "mountable::unix-device"}, // G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE
    {DFileInfo::AttributeID::MountableUnixDeviceFile, "mountable::unix-device-file"}, // G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE
    {DFileInfo::AttributeID::MountableHalUdi, "mountable::hal-udi"}, // G_FILE_ATTRIBUTE_MOUNTABLE_HAL_UDI
    {DFileInfo::AttributeID::MountableCanPoll, "mountable::can-poll"}, // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_POLL
    {DFileInfo::AttributeID::MountableIsMediaCheckAutomatic, "mountable::is-media-check-automatic"}, // G_FILE_ATTRIBUTE_MOUNTABLE_IS_MEDIA_CHECK_AUTOMATIC
    {DFileInfo::AttributeID::MountableCanStart, "mountable::can-start"}, // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START
    {DFileInfo::AttributeID::MountableCanStartDegraded, "mountable::can-start-degraded"}, // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START_DEGRADED
    {DFileInfo::AttributeID::MountableCanStop, "mountable::can-stop"}, // G_FILE_ATTRIBUTE_MOUNTABLE_CAN_STOP
    {DFileInfo::AttributeID::MountableStartStopType, "mountable::start-stop-type"}, // G_FILE_ATTRIBUTE_MOUNTABLE_START_STOP_TYPE

    {DFileInfo::AttributeID::TimeModified, "time::modified"}, // G_FILE_ATTRIBUTE_TIME_MODIFIED
    {DFileInfo::AttributeID::TimeModifiedUsec, "time::modified-usec"}, // G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC
    {DFileInfo::AttributeID::TimeAccess, "time::access"}, // G_FILE_ATTRIBUTE_TIME_ACCESS
    {DFileInfo::AttributeID::TimeAccessUsec, "time::access-usec"}, // G_FILE_ATTRIBUTE_TIME_ACCESS_USEC
    {DFileInfo::AttributeID::TimeChanged, "time::changed"}, // G_FILE_ATTRIBUTE_TIME_CHANGED
    {DFileInfo::AttributeID::TimeChangedUsec, "time::changed-usec"}, // G_FILE_ATTRIBUTE_TIME_CHANGED_USEC
    {DFileInfo::AttributeID::TimeCreated, "time::created"}, // G_FILE_ATTRIBUTE_TIME_CREATED
    {DFileInfo::AttributeID::TimeCreatedUsec, "time::created-usec"}, // G_FILE_ATTRIBUTE_TIME_CREATED_USEC

    {DFileInfo::AttributeID::UnixDevice, "unix::device"}, // G_FILE_ATTRIBUTE_UNIX_DEVICE
    {DFileInfo::AttributeID::UnixInode, "unix::inode"}, // G_FILE_ATTRIBUTE_UNIX_INODE
    {DFileInfo::AttributeID::UnixMode, "unix::mode"}, // G_FILE_ATTRIBUTE_UNIX_MODE
    {DFileInfo::AttributeID::UnixNlink, "unix::nlink"}, // G_FILE_ATTRIBUTE_UNIX_NLINK
    {DFileInfo::AttributeID::UnixUID, "unix::uid"}, // G_FILE_ATTRIBUTE_UNIX_UID
    {DFileInfo::AttributeID::UnixGID, "unix::gid"}, // G_FILE_ATTRIBUTE_UNIX_GID
    {DFileInfo::AttributeID::UnixRdev, "unix::rdev"}, // G_FILE_ATTRIBUTE_UNIX_RDEV
    {DFileInfo::AttributeID::UnixBlockSize, "unix::block-size"}, // G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE
    {DFileInfo::AttributeID::UnixBlocks, "unix::blocks"}, // G_FILE_ATTRIBUTE_UNIX_BLOCKS
    {DFileInfo::AttributeID::UnixIsMountPoint, "unix::is-mountpoint"}, // G_FILE_ATTRIBUTE_UNIX_IS_MOUNTPOINT

    {DFileInfo::AttributeID::DosIsArchive, "dos::is-archive"}, // G_FILE_ATTRIBUTE_DOS_IS_ARCHIVE
    {DFileInfo::AttributeID::DosIsSystem, "dos::is-system"}, // G_FILE_ATTRIBUTE_DOS_IS_SYSTEM

    {DFileInfo::AttributeID::OwnerUser, "owner::user"}, // G_FILE_ATTRIBUTE_OWNER_USER
    {DFileInfo::AttributeID::OwnerUserReal, "owner::user-real"}, // G_FILE_ATTRIBUTE_OWNER_USER_REAL
    {DFileInfo::AttributeID::OwnerGroup, "owner::group"}, // G_FILE_ATTRIBUTE_OWNER_GROUP

    {DFileInfo::AttributeID::ThumbnailPath, "thumbnail::path"}, // G_FILE_ATTRIBUTE_THUMBNAIL_PATH
    {DFileInfo::AttributeID::ThumbnailFailed, "thumbnail::failed"}, // G_FILE_ATTRIBUTE_THUMBNAILING_FAILED
    {DFileInfo::AttributeID::ThumbnailIsValid, "thumbnail::is-valid"}, // G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID

    {DFileInfo::AttributeID::PreviewIcon, "preview::icon"}, // G_FILE_ATTRIBUTE_PREVIEW_ICON

    {DFileInfo::AttributeID::FileSystemSize, "filesystem::size"}, // G_FILE_ATTRIBUTE_FILESYSTEM_SIZE
    {DFileInfo::AttributeID::FileSystemFree, "filesystem::free"}, // G_FILE_ATTRIBUTE_FILESYSTEM_FREE
    {DFileInfo::AttributeID::FileSystemUsed, "filesystem::used"}, // G_FILE_ATTRIBUTE_FILESYSTEM_USED
    {DFileInfo::AttributeID::FileSystemType, "filesystem::type"}, // G_FILE_ATTRIBUTE_FILESYSTEM_TYPE
    {DFileInfo::AttributeID::FileSystemReadOnly, "filesystem::readonly"}, // G_FILE_ATTRIBUTE_FILESYSTEM_READONLY
    {DFileInfo::AttributeID::FileSystemUsePreview, "filesystem::use-preview"}, // G_FILE_ATTRIBUTE_FILESYSTEM_USE_PREVIEW
    {DFileInfo::AttributeID::FileSystemRemote, "filesystem::remote"}, // G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE

    {DFileInfo::AttributeID::GvfsBackend, "gvfs::backend"}, // G_FILE_ATTRIBUTE_GVFS_BACKEND

    {DFileInfo::AttributeID::SelinuxContext, "selinux::context"}, // G_FILE_ATTRIBUTE_SELINUX_CONTEXT

    {DFileInfo::AttributeID::TrashItemCount, "trash::item-count"}, // G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT
    {DFileInfo::AttributeID::TrashDeletionDate, "trash::deletion-date"}, // G_FILE_ATTRIBUTE_TRASH_DELETION_DATE
    {DFileInfo::AttributeID::TrashOrigPath, "trash::orig-path"}, // G_FILE_ATTRIBUTE_TRASH_ORIG_PATH

    {DFileInfo::AttributeID::RecentModified, "recent::modified"}, // G_FILE_ATTRIBUTE_RECENT_MODIFIED

    {DFileInfo::AttributeID::CustomStart, "custom-start"},

    {DFileInfo::AttributeID::StandardIsFile, "standard::is-file"},
    {DFileInfo::AttributeID::StandardIsDir, "standard::is-dir"},
    {DFileInfo::AttributeID::StandardIsRoot, "standard::is-root"},
    {DFileInfo::AttributeID::StandardSuffix, "standard::suffix"},
    {DFileInfo::AttributeID::StandardCompleteSuffix, "standard::complete-suffix"},
    {DFileInfo::AttributeID::StandardFilePath, "standard::file-path"},
    {DFileInfo::AttributeID::StandardParentPath, "standard::parent-path"},
    {DFileInfo::AttributeID::StandardBaseName, "standard::base-name"},
    {DFileInfo::AttributeID::StandardFileName, "standard::file-name"},
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
    *success = false;

    if (d->attributeFunc)
        return d->attributeFunc(id, success);

    return QVariant();
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
    if(!d->existsFunc)
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

QUrl DFileInfo::uri() const
{
    return d->uri;
}

QString DFileInfo::dump() const
{
    QString ret;
    /*QMap<AttributeID, QVariant>::const_iterator iter = d_ptr->attributes.begin();
    while (iter != d_ptr->attributes.end()) {
        ret.append(attributeName(iter.key()));
        ret.append(":");
        ret.append(iter.value().toString());
        ret.append("\n");
        ++iter;
    }*/
    return ret;
}

DFMIOError DFileInfo::lastError() const
{
    return d->error;
}
