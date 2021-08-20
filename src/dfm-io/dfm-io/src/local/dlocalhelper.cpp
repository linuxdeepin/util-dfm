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

#include <QDebug>

USING_IO_NAMESPACE

QSharedPointer<DFileInfo> DLocalHelper::getFileInfo(const QString &path)
{
    qInfo() << path;
    GFile *file = g_file_new_for_uri(path.toStdString().c_str());
    if (!file)
        return nullptr;

    GError *error = nullptr;
    GFileInfo *gfileinfo = g_file_query_info(file, "*", G_FILE_QUERY_INFO_NONE, nullptr, &error);
    g_object_unref(file);

    if (error)
        g_error_free(error);

    if (!gfileinfo)
        return nullptr;

    QSharedPointer<DFileInfo> info = DLocalHelper::getFileInfoFromGFileInfo(gfileinfo);
    g_object_unref(gfileinfo);
    return info;
}

QSharedPointer<DFileInfo> DLocalHelper::getFileInfoFromGFileInfo(GFileInfo *gfileinfo)
{
    QSharedPointer<DFileInfo> info = QSharedPointer<DFileInfo>(new DFileInfo());

    // file-type
    GFileType file_type = g_file_info_get_file_type(gfileinfo);
    info->setAttribute(DFileInfo::AttributeID::StandardType, file_type);

    // name
    const char *name = g_file_info_get_name(gfileinfo);
    info->setAttribute(DFileInfo::AttributeID::StandardName, name);

    // display-name
    const char *display_name = g_file_info_get_display_name(gfileinfo);
    info->setAttribute(DFileInfo::AttributeID::StandardDisplayName, display_name);

    // uid
    info->setAttribute(
        DFileInfo::AttributeID::UnixUID,
        g_file_info_get_attribute_uint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_UID));

    // gid
    info->setAttribute(
        DFileInfo::AttributeID::UnixGID,
        g_file_info_get_attribute_uint32(gfileinfo, G_FILE_ATTRIBUTE_UNIX_GID));

    // size
    info->setAttribute(
        DFileInfo::AttributeID::StandardSize,
        qulonglong(g_file_info_get_size(gfileinfo)));

    // atime
    info->setAttribute(
        DFileInfo::AttributeID::TimeAccess,
        qulonglong(g_file_info_get_attribute_uint64(gfileinfo, G_FILE_ATTRIBUTE_TIME_ACCESS)));

    // mtime
    info->setAttribute(
        DFileInfo::AttributeID::TimeModified,
        qulonglong(g_file_info_get_attribute_uint64(gfileinfo, G_FILE_ATTRIBUTE_TIME_MODIFIED)));

    // ctime
    info->setAttribute(
        DFileInfo::AttributeID::TimeChanged,
        qulonglong(g_file_info_get_attribute_uint64(gfileinfo, G_FILE_ATTRIBUTE_TIME_CHANGED)));

    return info;
}

GFileInfo *DLocalHelper::getFileInfoFromDFileInfo(const DFileInfo &dfileinfo)
{
    GFileInfo *info = g_file_info_new();

    bool ok = false;
    // file-type
    const GFileType &file_type = GFileType(
        dfileinfo.attribute(DFileInfo::AttributeID::StandardType, ok)
            .toInt());
    if (ok) {
        g_file_info_set_file_type(info, file_type);
    } else {
        //return false;
    }

    // name
    const char *name = dfileinfo.attribute(DFileInfo::AttributeID::StandardName, ok).toString().toStdString().c_str();
    if (ok) {
        g_file_info_set_name(info, name);
    } else {
        //return false;
    }

    // display-name
    const char *display_name = dfileinfo.attribute(DFileInfo::AttributeID::StandardDisplayName, ok).toString().toStdString().c_str();
    if (ok) {
        g_file_info_set_display_name(info, display_name);
    } else {
        //return false;
    }

    // uid
    const uint32_t &unix_uid = dfileinfo.attribute(DFileInfo::AttributeID::UnixUID, ok).toUInt();
    if (ok) {
        g_file_info_set_attribute_uint32(info, G_FILE_ATTRIBUTE_UNIX_UID, unix_uid);
    } else {
        //return false;
    }

    // gid
    const uint32_t &unix_gid = dfileinfo.attribute(DFileInfo::AttributeID::UnixGID, ok).toUInt();
    if (ok) {
        g_file_info_set_attribute_uint32(info, G_FILE_ATTRIBUTE_UNIX_GID, unix_gid);
    } else {
        //return false;
    }

    // size
    const int &size = dfileinfo.attribute(DFileInfo::AttributeID::StandardSize, ok).toInt();
    if (ok) {
        g_file_info_set_size(info, size);
    } else {
        //return false;
    }

    // atime
    const qulonglong &a_time = dfileinfo.attribute(DFileInfo::AttributeID::TimeAccess, ok).toULongLong();
    if (ok) {
        g_file_info_set_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_ACCESS, a_time);
    } else {
        //return false;
    }

    // mtime
    const qulonglong &m_time = dfileinfo.attribute(DFileInfo::AttributeID::TimeModified, ok).toULongLong();
    if (ok) {
        g_file_info_set_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED, m_time);
    } else {
        //return false;
    }

    // ctime
    const qulonglong &c_time = dfileinfo.attribute(DFileInfo::AttributeID::TimeChanged, ok).toULongLong();
    if (ok) {
        g_file_info_set_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_CHANGED, c_time);
    } else {
        //return false;
    }

    return info;
}
