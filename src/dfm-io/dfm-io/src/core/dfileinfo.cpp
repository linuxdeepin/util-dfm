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
#include "utils/dmediainfo.h"

USING_IO_NAMESPACE

DFileInfo::AttributeInfoMap DFileInfo::attributeInfoMap = {
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

    { DFileInfo::AttributeID::kAccessCanRead, std::make_tuple<std::string, QVariant>("access::can-read", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_READ
    { DFileInfo::AttributeID::kAccessCanWrite, std::make_tuple<std::string, QVariant>("access::can-write", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE
    { DFileInfo::AttributeID::kAccessCanExecute, std::make_tuple<std::string, QVariant>("access::can-execute", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE
    { DFileInfo::AttributeID::kAccessCanDelete, std::make_tuple<std::string, QVariant>("access::can-delete", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE
    { DFileInfo::AttributeID::kAccessCanTrash, std::make_tuple<std::string, QVariant>("access::can-trash", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH
    { DFileInfo::AttributeID::kAccessCanRename, std::make_tuple<std::string, QVariant>("access::can-rename", false) },   // G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME

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

DFileInfo::DFileInfo()
    : d(new DFileInfoPrivate(this))
{
}

DFileInfo::DFileInfo(const QUrl &uri, const char *attributes, const FileQueryInfoFlags flag)
    : d(new DFileInfoPrivate(this))
{
    d->uri = uri;
    d->attributes = strdup(attributes);
    d->flag = flag;
}

DFileInfo::DFileInfo(const DFileInfo &info)
    : d(info.d)
{
}

DFileInfo::~DFileInfo()
{
    free(d->attributes);
}

DFileInfo &DFileInfo::operator=(const DFileInfo &info)
{
    d = info.d;
    return *this;
}

void DFileInfo::queryInfoAsync(int ioPriority, DFileInfo::QueryInfoAsyncCallback func, void *userData) const
{
    if (d->queryInfoAsyncFunc)
        d->queryInfoAsyncFunc(ioPriority, func, userData);
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
 * @brief refresh attribute by @setAttribute to disk
 * @param
 * @return
 */
bool DFileInfo::refresh()
{
    if (d->refreshFunc)
        return d->refreshFunc();
    return false;
}

bool DFileInfo::clearCache()
{
    if (d->clearCacheFunc)
        return d->clearCacheFunc();
    return false;
}

DFile::Permissions DFileInfo::permissions()
{
    if (d->permissionFunc)
        return d->permissionFunc();
    return DFile::Permission::kNoPermission;
}

bool DFileInfo::setCustomAttribute(const char *key, const DFileAttributeType type, const void *value, const FileQueryInfoFlags flag)
{
    if (!d->setCustomAttributeFunc)
        return false;

    return d->setCustomAttributeFunc(key, type, value, flag);
}

QVariant DFileInfo::customAttribute(const char *key, const DFileInfo::DFileAttributeType type)
{
    if (!d->customAttributeFunc)
        return false;

    return d->customAttributeFunc(key, type);
}

void DFileInfo::attributeExtend(DFileInfo::MediaType type, QList<AttributeExtendID> ids, DFileInfo::AttributeExtendFuncCallback callback)
{
    d->attributeExtend(type, ids, callback);
}

bool DFileInfo::cancelAttributeExtend()
{
    return d->cancelAttributeExtend();
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

void DFileInfo::registerRefresh(const DFileInfo::RefreshFunc &func)
{
    d->refreshFunc = func;
}

void DFileInfo::registerClearCache(const DFileInfo::ClearCacheFunc &func)
{
    d->clearCacheFunc = func;
}

void DFileInfo::registerPermissions(const DFile::PermissionFunc &func)
{
    d->permissionFunc = func;
}

void DFileInfo::registerSetCustomAttribute(const DFileInfo::SetCustomAttributeFunc &func)
{
    d->setCustomAttributeFunc = func;
}

void DFileInfo::registerCustomAttribute(const DFileInfo::CustomAttributeFunc &func)
{
    d->customAttributeFunc = func;
}

void DFileInfo::registerLastError(const DFileInfo::LastErrorFunc &func)
{
    d->lastErrorFunc = func;
}

void DFileInfo::registerQueryInfoAsync(const DFileInfo::QueryInfoAsyncFunc &func)
{
    d->queryInfoAsyncFunc = func;
}

QUrl DFileInfo::uri() const
{
    return d->uri;
}

char *DFileInfo::queryAttributes() const
{
    return d->attributes;
}

DFileInfo::FileQueryInfoFlags DFileInfo::queryInfoFlag() const
{
    return d->flag;
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

void DFileInfoPrivate::attributeExtend(DFileInfo::MediaType type, QList<DFileInfo::AttributeExtendID> ids, DFileInfo::AttributeExtendFuncCallback callback)
{
    if (ids.contains(DFileInfo::AttributeExtendID::kExtendMediaDuration)
        || ids.contains(DFileInfo::AttributeExtendID::kExtendMediaWidth)
        || ids.contains(DFileInfo::AttributeExtendID::kExtendMediaHeight)) {

        const QString &filePath = attributeFunc ? attributeFunc(DFileInfo::AttributeID::kStandardFilePath, nullptr).toString() : QString();
        if (!filePath.isEmpty()) {
            mediaType = type;
            extendIDs = ids;
            attributeExtendFuncCallback = callback;

            if (!this->mediaInfo) {
                this->mediaInfo.reset(new DMediaInfo(filePath));
            }
            this->mediaInfo->startReadInfo(std::bind(&DFileInfoPrivate::attributeExtendCallback, this));
        } else {
            if (callback)
                callback(false, {});
        }
    }
}

bool DFileInfoPrivate::cancelAttributeExtend()
{
    if (this->mediaInfo)
        this->mediaInfo->stopReadInfo();
    return true;
}

void DFileInfoPrivate::attributeExtendCallback()
{
    if (this->mediaInfo) {
        QMap<DFileInfo::AttributeExtendID, QVariant> map;

        if (extendIDs.contains(DFileInfo::AttributeExtendID::kExtendMediaDuration)) {
            QString duration = mediaInfo->value("Duration", mediaType);
            if (duration.isEmpty()) {
                duration = mediaInfo->value("Duration", DFileInfo::MediaType::kGeneral);
            }
            map.insert(DFileInfo::AttributeExtendID::kExtendMediaDuration, duration);
        }
        if (extendIDs.contains(DFileInfo::AttributeExtendID::kExtendMediaWidth)) {
            const QString &width = mediaInfo->value("Width", mediaType);
            map.insert(DFileInfo::AttributeExtendID::kExtendMediaWidth, width);
        }
        if (extendIDs.contains(DFileInfo::AttributeExtendID::kExtendMediaHeight)) {
            const QString &height = mediaInfo->value("Height", mediaType);
            map.insert(DFileInfo::AttributeExtendID::kExtendMediaHeight, height);
        }

        if (attributeExtendFuncCallback)
            attributeExtendFuncCallback(true, map);
    }
}
