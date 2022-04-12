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

bool isFile(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toLocal8Bit().data());
    const bool ret = DLocalHelper::checkGFileType(file, G_FILE_TYPE_REGULAR);
    return ret;
}

bool isDir(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toLocal8Bit().data());
    const bool ret = DLocalHelper::checkGFileType(file, G_FILE_TYPE_DIRECTORY);
    return ret;
}

bool isSymlink(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toLocal8Bit().data());
    const bool ret = DLocalHelper::checkGFileType(file, G_FILE_TYPE_SYMBOLIC_LINK);
    return ret;
}

bool isRoot(const QString &path)
{
    return path == "/";
}

QString fileName(const QString &path)
{
    g_autoptr(GFile) gfile = g_file_new_for_path(path.toLocal8Bit().data());
    g_autofree gchar *baseName = g_file_get_basename(gfile);
    return QString::fromLocal8Bit(baseName);
}

QString baseName(const QString &path)
{
    const QString &fullName = fileName(path);

    if (isDir(path))
        return fullName;

    int pos2 = fullName.indexOf(".");
    if (pos2 == -1)
        return fullName;
    else
        return fullName.left(pos2);
}

QString completeBaseName(const QString &path)
{
    const QString &fullName = fileName(path);

    if (isDir(path))
        return fullName;

    int pos2 = fullName.lastIndexOf(".");
    if (pos2 == -1)
        return fullName;
    else
        return fullName.left(pos2);
}

QString suffix(const QString &path)
{
    // path
    if (isDir(path))
        return "";

    const QString &fullName = fileName(path);

    int pos2 = fullName.lastIndexOf(".");
    if (pos2 == -1)
        return "";
    else
        return fullName.mid(pos2 + 1);
}

QString completeSuffix(const QString &path)
{
    if (isDir(path))
        return "";

    const QString &fullName = fileName(path);

    int pos2 = fullName.indexOf(".");
    if (pos2 == -1)
        return "";
    else
        return fullName.mid(pos2 + 1);
}

bool exists(const QString &path)
{
    g_autoptr(GFile) gfile = g_file_new_for_path(path.toStdString().c_str());
    const bool exists = g_file_query_exists(gfile, nullptr);

    return exists;
}

QString filePath(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toLocal8Bit().data());

    g_autofree gchar *gpath = g_file_get_path(file);
    QString retPath = QString::fromLocal8Bit(gpath);

    return retPath;
}

QString parentPath(const QString &path)
{
    g_autoptr(GFile) file = g_file_new_for_path(path.toLocal8Bit().data());
    g_autoptr(GFile) fileParent = g_file_get_parent(file);

    g_autofree gchar *gpath = g_file_get_path(fileParent);
    QString retPath = QString::fromLocal8Bit(gpath);

    return retPath;
}
}

QSharedPointer<DFileInfo> DLocalHelper::createFileInfoByUri(const QUrl &uri)
{
    return QSharedPointer<DFileInfo>(new DLocalFileInfo(uri));
}

QVariant DLocalHelper::attributeFromGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, DFMIOErrorCode &errorcode)
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
    if (!hasAttr) {
        errorcode = DFM_IO_ERROR_INFO_NO_ATTRIBUTE;
        return QVariant();
    }

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
    case DFileInfo::AttributeID::TrashItemCount:
    case DFileInfo::AttributeID::ExtendWordSize:
    case DFileInfo::AttributeID::ExtendMediaDuration: {
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
    case DFileInfo::AttributeID::StandardSymbolicIcon: {
        QList<QString> ret;

        GObject *icon = g_file_info_get_attribute_object(gfileinfo, key.c_str());
        auto names = g_themed_icon_get_names(G_THEMED_ICON(icon));
        for (int j = 0; names && names[j] != nullptr; ++j)
            ret.append(QString::fromLocal8Bit(names[j]));

        return QVariant(ret);
    }
    case DFileInfo::AttributeID::PreviewIcon: {
        GObject *ret = g_file_info_get_attribute_object(gfileinfo, key.c_str());
        Q_UNUSED(ret);
        // TODO(lanxs)
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
    case DFileInfo::AttributeID::StandardCompleteBaseName: {
        return LocalFunc::completeBaseName(path);
    }
    default:
        return QVariant();
    }
}

bool DLocalHelper::setAttributeByGFile(GFile *gfile, DFileInfo::AttributeID id, const QVariant &value, GError **gerror)
{
    if (!gfile) {
        return false;
    }

    // check has attribute
    const std::string &key = DLocalHelper::attributeStringById(id);
    if (key.empty()) {
        return false;
    }

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
    case DFileInfo::AttributeID::TrashItemCount:
    case DFileInfo::AttributeID::ExtendWordSize:
    case DFileInfo::AttributeID::ExtendMediaDuration: {
        g_file_set_attribute_uint32(gfile, key.c_str(), value.toUInt(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;

            return false;
        }
        return true;
    }
    // int32_t
    case DFileInfo::AttributeID::StandardSortOrder: {
        g_file_set_attribute_int32(gfile, key.c_str(), value.toInt(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
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
        g_file_set_attribute_uint64(gfile, key.c_str(), value.toULongLong(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
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
        gboolean b = value.toBool();
        gpointer gpValue = &b;
        g_file_set_attribute(gfile, key.c_str(), G_FILE_ATTRIBUTE_TYPE_BOOLEAN, gpValue, G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
    }
    // byte string
    case DFileInfo::AttributeID::StandardName:
    case DFileInfo::AttributeID::StandardSymlinkTarget:
    case DFileInfo::AttributeID::ThumbnailPath: {
        g_file_set_attribute_byte_string(gfile, key.c_str(), value.toString().toLocal8Bit().data(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
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
        g_file_set_attribute_string(gfile, key.c_str(), value.toString().toLocal8Bit().data(), G_FILE_QUERY_INFO_NONE, nullptr, gerror);
        if (gerror) {
            g_autofree gchar *url = g_file_get_uri(gfile);
            //qWarning() << "file set attribute failed, url: " << url << " msg: " << (*gerror)->message;
            return false;
        }
        return true;
    }
    // object
    case DFileInfo::AttributeID::StandardIcon:
    case DFileInfo::AttributeID::StandardSymbolicIcon:
    case DFileInfo::AttributeID::PreviewIcon: {
        //g_file_info_set_attribute_object(gfileinfo, key, value.object());
        // TODO(lanxs)
        return true;
    }

    default:
        return true;
    }
}

bool DLocalHelper::setAttributeByGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, const QVariant &value)
{
    if (!gfileinfo) {
        return false;
    }

    // check has attribute
    const std::string &key = DLocalHelper::attributeStringById(id);
    if (key.empty()) {
        return false;
    }

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
    case DFileInfo::AttributeID::TrashItemCount:
    case DFileInfo::AttributeID::ExtendWordSize:
    case DFileInfo::AttributeID::ExtendMediaDuration: {
        g_file_info_set_attribute_uint32(gfileinfo, key.c_str(), value.toUInt());
        return true;
    }
    // int32_t
    case DFileInfo::AttributeID::StandardSortOrder: {
        g_file_info_set_attribute_int32(gfileinfo, key.c_str(), value.toInt());
        return true;
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
        g_file_info_set_attribute_uint64(gfileinfo, key.c_str(), value.toULongLong());
        return true;
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
        g_file_info_set_attribute_boolean(gfileinfo, key.c_str(), value.toBool());
        return true;
    }
    // byte string
    case DFileInfo::AttributeID::StandardName:
    case DFileInfo::AttributeID::StandardSymlinkTarget:
    case DFileInfo::AttributeID::ThumbnailPath: {
        g_file_info_set_attribute_byte_string(gfileinfo, key.c_str(), value.toString().toLocal8Bit().data());
        return true;
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
        g_file_info_set_attribute_string(gfileinfo, key.c_str(), value.toString().toLocal8Bit().data());
        return true;
    }
    // object
    case DFileInfo::AttributeID::StandardIcon:
    case DFileInfo::AttributeID::StandardSymbolicIcon:
    case DFileInfo::AttributeID::PreviewIcon: {
        //g_file_info_set_attribute_object(gfileinfo, key, value.object());
        // TODO(lanxs)
        return true;
    }

    default:
        return true;
    }
}

std::string DLocalHelper::attributeStringById(DFileInfo::AttributeID id)
{
    if (DFileInfo::attributeInfoMap.count(id) > 0) {
        const std::string &value = std::get<0>(DFileInfo::attributeInfoMap.at(id));
        return value;
    }
    return "";
}

QSet<QString> DLocalHelper::hideListFromUrl(const QUrl &url)
{
    g_autofree char *contents = nullptr;
    g_autoptr(GError) error = nullptr;
    gsize len = 0;
    g_autoptr(GFile) hiddenFile = g_file_new_for_uri(url.toString().toLocal8Bit().data());
    const bool succ = g_file_load_contents(hiddenFile, nullptr, &contents, &len, nullptr, &error);
    if (succ) {
        if (contents && len > 0) {
            QString dataStr(contents);
            return QSet<QString>::fromList(dataStr.split('\n', QString::SkipEmptyParts));
        }
    } else {
        qWarning() << "load .hidden fail, code: " << error->code << " " << QString::fromLocal8Bit(error->message);
    }
    return {};
}

bool DLocalHelper::fileIsHidden(const QSharedPointer<DFileInfo> &dfileinfo, const QSet<QString> &hideList)
{
    if (!dfileinfo)
        return false;

    const QString &fileName = dfileinfo->attribute(DFileInfo::AttributeID::StandardName, nullptr).toString();
    if (fileName.startsWith(".")) {
        return true;
    } else {
        if (hideList.isEmpty()) {
            const QString &hiddenPath = dfileinfo->attribute(DFileInfo::AttributeID::StandardParentPath, nullptr).toString() + "/.hidden";
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
    GError *gerror = nullptr;
    GFileInfo *gfileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, nullptr, &gerror);

    if (gerror)
        g_error_free(gerror);

    if (!gfileinfo)
        return false;

    bool ret = g_file_info_get_file_type(gfileinfo) == type;
    g_object_unref(gfileinfo);
    return ret;
}
