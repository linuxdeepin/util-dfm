/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#include "local/dlocalhelper.h"
#include "core/dfileinfo.h"
#include "local/dlocalfileinfo.h"

#include "gio/gfileinfo.h"

#include <QDebug>

USING_IO_NAMESPACE

namespace LocalFunc {
    QString fileName(const QString &path)
    {
        int pos = path.lastIndexOf("/");
        if (pos == -1) {
            return path;
        } else {
            return path.mid(pos + 1);
        }
    }

    QString baseName(const QString &path)
    {
        const QString &fullName = fileName(path);

        int pos2 = fullName.indexOf(".");
        if (pos2 == -1)
            return fullName;
        else
            return fullName.left(pos2);
    }

    QString suffix(const QString &path)
    {
        const QString &fullName = fileName(path);

        int pos2 = fullName.lastIndexOf(".");
        if (pos2 == -1)
            return fullName;
        else
            return fullName.mid(pos2 + 1);
    }

    QString completeSuffix(const QString &path)
    {
        const QString &fullName = fileName(path);

        int pos2 = fullName.indexOf(".");
        if (pos2 == -1)
            return fullName;
        else
            return fullName.mid(pos2 + 1);
    }

    bool exists(const QString &path)
    {
        GFile *gfile = g_file_new_for_path(path.toStdString().c_str());
        const bool exists = g_file_query_exists(gfile, nullptr);

        g_object_unref(gfile);
        return exists;
    }

    QString filePath(const QString &path)
    {
        GFile *file = g_file_new_for_path(path.toLocal8Bit().data());

        QString retPath = QString::fromLocal8Bit(g_file_get_path(file));

        g_object_unref(file);

        return retPath;
    }

    QString parentPath(const QString &path)
    {
        GFile *file = g_file_new_for_path(path.toLocal8Bit().data());
        GFile *fileParent = g_file_get_parent(file);

        QString retPath = QString::fromLocal8Bit(g_file_get_path(fileParent));

        g_object_unref(file);
        g_object_unref(fileParent);

        return retPath;
    }

    bool checkFileType(const QString &path, GFileType type)
    {
        GFile *file = g_file_new_for_path(path.toLocal8Bit().data());

        GError *error = nullptr;
        GFileInfo *gfileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, nullptr, &error);
        g_object_unref(file);

        if (error)
            g_error_free(error);

        if (!gfileinfo)
            return false;

        bool ret = g_file_info_get_file_type(gfileinfo) == type;
        g_object_unref(gfileinfo);
        return ret;
    }

    bool isFile(const QString &path)
    {
        return checkFileType(path, G_FILE_TYPE_REGULAR);
    }

    bool isDir(const QString &path)
    {
        return checkFileType(path, G_FILE_TYPE_DIRECTORY);
    }

    bool isSymlink(const QString &path)
    {
        return checkFileType(path, G_FILE_TYPE_SYMBOLIC_LINK);
    }

    bool isRoot(const QString &path)
    {
        return path == "/";
    }
}

QSharedPointer<DFileInfo> DLocalHelper::getFileInfo(const QString &uri)
{
    return DLocalHelper::getFileInfoByUri(uri);
}

QSharedPointer<DFileInfo> DLocalHelper::getFileInfoByUri(const QString &uri)
{
    QSharedPointer<DFileInfo> info = QSharedPointer<DFileInfo>(new DLocalFileInfo(QUrl(uri)));
    return info;
}

GFileInfo *DLocalHelper::getFileInfoFromDFileInfo(const DFileInfo &dfileinfo)
{
    GFileInfo *info = g_file_info_new();

    for (const auto &[key, value] : DFileInfo::attributeNames) {
        setAttributeByGFileInfo(info, key, dfileinfo.attribute(key, nullptr));
    }

    return info;
}

QVariant DLocalHelper::attributeFromGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id)
{
    if (!gfileinfo)
        return QVariant();

    // check custom attribute
    if (id > DFileInfo::AttributeID::CustomStart) {
        return QVariant();
    }

    // check has attribute
    const std::string &key = DLocalHelper::attributeStringById(id);
    bool hasAttr = g_file_info_has_attribute(gfileinfo, key.c_str());
    if (!hasAttr)
        return QVariant();

    switch (id) {
    // uint32_t
    case DFileInfo::AttributeID::StandardType:
    case DFileInfo::AttributeID::MountableUnixDevice:
    case DFileInfo::AttributeID::MountableStartStopType:
    case DFileInfo::AttributeID::TimeModifiedUsec:
    case DFileInfo::AttributeID::TimeAccessUsec:
    case DFileInfo::AttributeID::TimeChangedUsec:
    case DFileInfo::AttributeID::TimeCreatedUsec:
    case DFileInfo::AttributeID::UnixDevice:
    case DFileInfo::AttributeID::UnixMode:
    case DFileInfo::AttributeID::UnixNlink:
    case DFileInfo::AttributeID::UnixUID:
    case DFileInfo::AttributeID::UnixGID:
    case DFileInfo::AttributeID::UnixRdev:
    case DFileInfo::AttributeID::UnixBlockSize:
    case DFileInfo::AttributeID::FileSystemUsePreview:
    case DFileInfo::AttributeID::TrashItemCount: {
        uint32_t ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // int32_t
    case DFileInfo::AttributeID::StandardSortOrder: {
        int32_t ret = g_file_info_get_attribute_int32(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // uint64_t
    case DFileInfo::AttributeID::StandardSize:
    case DFileInfo::AttributeID::StandardAllocatedSize:
    case DFileInfo::AttributeID::TimeModified:
    case DFileInfo::AttributeID::TimeAccess:
    case DFileInfo::AttributeID::TimeChanged:
    case DFileInfo::AttributeID::TimeCreated:
    case DFileInfo::AttributeID::UnixInode:
    case DFileInfo::AttributeID::UnixBlocks:
    case DFileInfo::AttributeID::FileSystemSize:
    case DFileInfo::AttributeID::FileSystemFree:
    case DFileInfo::AttributeID::FileSystemUsed:
    case DFileInfo::AttributeID::RecentModified: {
        uint64_t ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        return QVariant(qulonglong(ret));
    }
    // bool
    case DFileInfo::AttributeID::StandardIsHidden:
    case DFileInfo::AttributeID::StandardIsBackup:
    case DFileInfo::AttributeID::StandardIsSymlink:
    case DFileInfo::AttributeID::StandardIsVirtual:
    case DFileInfo::AttributeID::StandardIsVolatile:
    case DFileInfo::AttributeID::AccessCanRead:
    case DFileInfo::AttributeID::AccessCanWrite:
    case DFileInfo::AttributeID::AccessCanExecute:
    case DFileInfo::AttributeID::AccessCanDelete:
    case DFileInfo::AttributeID::AccessCanTrash:
    case DFileInfo::AttributeID::AccessCanRename:
    case DFileInfo::AttributeID::MountableCanMount:
    case DFileInfo::AttributeID::MountableCanUnmount:
    case DFileInfo::AttributeID::MountableCanEject:
    case DFileInfo::AttributeID::MountableCanPoll:
    case DFileInfo::AttributeID::MountableIsMediaCheckAutomatic:
    case DFileInfo::AttributeID::MountableCanStart:
    case DFileInfo::AttributeID::MountableCanStartDegraded:
    case DFileInfo::AttributeID::MountableCanStop:
    case DFileInfo::AttributeID::UnixIsMountPoint:
    case DFileInfo::AttributeID::DosIsArchive:
    case DFileInfo::AttributeID::DosIsSystem:
    case DFileInfo::AttributeID::FileSystemReadOnly:
    case DFileInfo::AttributeID::FileSystemRemote: {
        bool ret = g_file_info_get_attribute_boolean(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // byte string
    case DFileInfo::AttributeID::StandardName:
    case DFileInfo::AttributeID::StandardSymlinkTarget:
    case DFileInfo::AttributeID::ThumbnailPath: {
        const char *ret = g_file_info_get_attribute_byte_string(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // string
    case DFileInfo::AttributeID::StandardDisplayName:
    case DFileInfo::AttributeID::StandardEditName:
    case DFileInfo::AttributeID::StandardCopyName:
    case DFileInfo::AttributeID::StandardContentType:
    case DFileInfo::AttributeID::StandardFastContentType:
    case DFileInfo::AttributeID::StandardTargetUri:
    case DFileInfo::AttributeID::StandardDescription:
    case DFileInfo::AttributeID::EtagValue:
    case DFileInfo::AttributeID::IdFile:
    case DFileInfo::AttributeID::IdFilesystem:
    case DFileInfo::AttributeID::MountableUnixDeviceFile:
    case DFileInfo::AttributeID::MountableHalUdi:
    case DFileInfo::AttributeID::OwnerUser:
    case DFileInfo::AttributeID::OwnerUserReal:
    case DFileInfo::AttributeID::OwnerGroup:
    case DFileInfo::AttributeID::FileSystemType:
    case DFileInfo::AttributeID::GvfsBackend:
    case DFileInfo::AttributeID::SelinuxContext:
    case DFileInfo::AttributeID::TrashDeletionDate:
    case DFileInfo::AttributeID::TrashOrigPath: {
        const char *ret = g_file_info_get_attribute_string(gfileinfo, key.c_str());
        return QVariant(ret);
    }
    // object
    case DFileInfo::AttributeID::StandardIcon:
    case DFileInfo::AttributeID::StandardSymbolicIcon:
    case DFileInfo::AttributeID::PreviewIcon: {
        GObject *ret = g_file_info_get_attribute_object(gfileinfo, key.c_str());
        Q_UNUSED(ret);
        // TODO
        return QVariant();
    }

    default:
        return QVariant();
    }
}

QVariant DLocalHelper::customAttributeFromPath(const QString &path, DFileInfo::AttributeID id)
{
    if (id < DFileInfo::AttributeID::CustomStart)
        return QVariant();

    switch (id) {
    case DFileInfo::AttributeID::StandardIsFile: {
        return LocalFunc::isFile(path);
    }
    case DFileInfo::AttributeID::StandardIsDir: {
        return LocalFunc::isDir(path);
    }
    case DFileInfo::AttributeID::StandardIsRoot: {
        return LocalFunc::isRoot(path);
    }
    case DFileInfo::AttributeID::StandardSuffix: {
        return LocalFunc::suffix(path);
    }
    case DFileInfo::AttributeID::StandardCompleteSuffix: {
        return LocalFunc::completeSuffix(path);
    }
    case DFileInfo::AttributeID::StandardFilePath: {
        return LocalFunc::filePath(path);
    }
    case DFileInfo::AttributeID::StandardParentPath: {
        return LocalFunc::parentPath(path);
    }
    case DFileInfo::AttributeID::StandardBaseName: {
        return LocalFunc::baseName(path);
    }
    case DFileInfo::AttributeID::StandardFileName: {
        return LocalFunc::fileName(path);
    }
    default:
        return QVariant();
    }
}

void DLocalHelper::setAttributeByGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, const QVariant &value)
{
    if (!gfileinfo)
        return;

    // check has attribute
    const char *key = DLocalHelper::attributeStringById(id).c_str();
    bool hasAttr = g_file_info_has_attribute(gfileinfo, key);
    if (!hasAttr)
        return;

    switch (id) {
    // uint32_t
    case DFileInfo::AttributeID::StandardType:
    case DFileInfo::AttributeID::MountableUnixDevice:
    case DFileInfo::AttributeID::MountableStartStopType:
    case DFileInfo::AttributeID::TimeModifiedUsec:
    case DFileInfo::AttributeID::TimeAccessUsec:
    case DFileInfo::AttributeID::TimeChangedUsec:
    case DFileInfo::AttributeID::TimeCreatedUsec:
    case DFileInfo::AttributeID::UnixDevice:
    case DFileInfo::AttributeID::UnixMode:
    case DFileInfo::AttributeID::UnixNlink:
    case DFileInfo::AttributeID::UnixUID:
    case DFileInfo::AttributeID::UnixGID:
    case DFileInfo::AttributeID::UnixRdev:
    case DFileInfo::AttributeID::UnixBlockSize:
    case DFileInfo::AttributeID::FileSystemUsePreview:
    case DFileInfo::AttributeID::TrashItemCount: {
        g_file_info_set_attribute_uint32(gfileinfo, key, value.toUInt());
        return;
    }
    // int32_t
    case DFileInfo::AttributeID::StandardSortOrder: {
        g_file_info_set_attribute_int32(gfileinfo, key, value.toInt());
        return;
    }
    // uint64_t
    case DFileInfo::AttributeID::StandardSize:
    case DFileInfo::AttributeID::StandardAllocatedSize:
    case DFileInfo::AttributeID::TimeModified:
    case DFileInfo::AttributeID::TimeAccess:
    case DFileInfo::AttributeID::TimeChanged:
    case DFileInfo::AttributeID::TimeCreated:
    case DFileInfo::AttributeID::UnixInode:
    case DFileInfo::AttributeID::UnixBlocks:
    case DFileInfo::AttributeID::FileSystemSize:
    case DFileInfo::AttributeID::FileSystemFree:
    case DFileInfo::AttributeID::FileSystemUsed:
    case DFileInfo::AttributeID::RecentModified: {
        g_file_info_set_attribute_uint64(gfileinfo, key, value.toULongLong());
        return;
    }
    // bool
    case DFileInfo::AttributeID::StandardIsHidden:
    case DFileInfo::AttributeID::StandardIsBackup:
    case DFileInfo::AttributeID::StandardIsSymlink:
    case DFileInfo::AttributeID::StandardIsVirtual:
    case DFileInfo::AttributeID::StandardIsVolatile:
    case DFileInfo::AttributeID::AccessCanRead:
    case DFileInfo::AttributeID::AccessCanWrite:
    case DFileInfo::AttributeID::AccessCanExecute:
    case DFileInfo::AttributeID::AccessCanDelete:
    case DFileInfo::AttributeID::AccessCanTrash:
    case DFileInfo::AttributeID::AccessCanRename:
    case DFileInfo::AttributeID::MountableCanMount:
    case DFileInfo::AttributeID::MountableCanUnmount:
    case DFileInfo::AttributeID::MountableCanEject:
    case DFileInfo::AttributeID::MountableCanPoll:
    case DFileInfo::AttributeID::MountableIsMediaCheckAutomatic:
    case DFileInfo::AttributeID::MountableCanStart:
    case DFileInfo::AttributeID::MountableCanStartDegraded:
    case DFileInfo::AttributeID::MountableCanStop:
    case DFileInfo::AttributeID::UnixIsMountPoint:
    case DFileInfo::AttributeID::DosIsArchive:
    case DFileInfo::AttributeID::DosIsSystem:
    case DFileInfo::AttributeID::FileSystemReadOnly:
    case DFileInfo::AttributeID::FileSystemRemote: {
        g_file_info_set_attribute_boolean(gfileinfo, key, value.toBool());
        return;
    }
    // byte string
    case DFileInfo::AttributeID::StandardName:
    case DFileInfo::AttributeID::StandardSymlinkTarget:
    case DFileInfo::AttributeID::ThumbnailPath: {
        g_file_info_set_attribute_byte_string(gfileinfo, key, value.toString().toLocal8Bit().data());
        return;
    }
    // string
    case DFileInfo::AttributeID::StandardDisplayName:
    case DFileInfo::AttributeID::StandardEditName:
    case DFileInfo::AttributeID::StandardCopyName:
    case DFileInfo::AttributeID::StandardContentType:
    case DFileInfo::AttributeID::StandardFastContentType:
    case DFileInfo::AttributeID::StandardTargetUri:
    case DFileInfo::AttributeID::StandardDescription:
    case DFileInfo::AttributeID::EtagValue:
    case DFileInfo::AttributeID::IdFile:
    case DFileInfo::AttributeID::IdFilesystem:
    case DFileInfo::AttributeID::MountableUnixDeviceFile:
    case DFileInfo::AttributeID::MountableHalUdi:
    case DFileInfo::AttributeID::OwnerUser:
    case DFileInfo::AttributeID::OwnerUserReal:
    case DFileInfo::AttributeID::OwnerGroup:
    case DFileInfo::AttributeID::FileSystemType:
    case DFileInfo::AttributeID::GvfsBackend:
    case DFileInfo::AttributeID::SelinuxContext:
    case DFileInfo::AttributeID::TrashDeletionDate:
    case DFileInfo::AttributeID::TrashOrigPath: {
        g_file_info_set_attribute_string(gfileinfo, key, value.toString().toLocal8Bit().data());
        return;
    }
    // object
    case DFileInfo::AttributeID::StandardIcon:
    case DFileInfo::AttributeID::StandardSymbolicIcon:
    case DFileInfo::AttributeID::PreviewIcon: {
        //g_file_info_set_attribute_object(gfileinfo, key, value.object());
        // TODO
        return;
    }

    default:
        return;
    }
}

std::string DLocalHelper::attributeStringById(DFileInfo::AttributeID id)
{
    if (DFileInfo::attributeNames.count(id) > 0) {
        const std::string &value = DFileInfo::attributeNames.at(id);
        return value;
    }
    return "";
}
