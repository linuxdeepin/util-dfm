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

#include "local/dlocalfileinfo.h"
#include "local/dlocalfileinfo_p.h"
#include "local/dlocalhelper.h"

#include <QDebug>

#include <sys/stat.h>

USING_IO_NAMESPACE

DLocalFileInfoPrivate::DLocalFileInfoPrivate(DLocalFileInfo *q)
    : q(q)
{

}

DLocalFileInfoPrivate::~DLocalFileInfoPrivate()
{
}

bool DLocalFileInfoPrivate::init()
{
    const QUrl &url = q->uri();
    const QString &path = url.toString();

    GFile *file = g_file_new_for_uri(path.toLocal8Bit().data());

    GError *gerror = nullptr;

    GFileInfo *gfileinfo = g_file_query_info(file, "*", G_FILE_QUERY_INFO_NONE, nullptr, &gerror);
    g_object_unref(file);

    if (gerror)
        g_error_free(gerror);

    if (!gfileinfo)
        return false;

    this->gfileinfo = gfileinfo;
    return true;
}

QVariant DLocalFileInfoPrivate::attribute(DFileInfo::AttributeID id, bool *success)
{
    QVariant retValue;
    if (attributes.count(id) == 0) {
        if (gfileinfo) {
            if (id > DFileInfo::AttributeID::CustomStart) {
                const QString &path = q->uri().path();
                retValue = DLocalHelper::customAttributeFromPath(path, id);
            } else {
                retValue = DLocalHelper::attributeFromGFileInfo(gfileinfo, id);
            }
            setAttribute(id, retValue);
        }
        if (success)
            *success = retValue.isValid();
        return retValue;
    }
    if (success)
        *success = true;
    return attributes.value(id);
}

bool DLocalFileInfoPrivate::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    if (attributes.count(id) > 0)
        attributes.remove(id);

    attributes.insert(id, value);
    return true;
}

bool DLocalFileInfoPrivate::hasAttribute(DFileInfo::AttributeID id)
{
    if (attributes.count(id) > 0)
        return true;

    if (gfileinfo) {
        const QVariant &value = DLocalHelper::attributeFromGFileInfo(gfileinfo, id);
        setAttribute(id, value);
        return true;
    }
    return false;
}

bool DLocalFileInfoPrivate::removeAttribute(DFileInfo::AttributeID id)
{
    if (attributes.count(id) > 0)
        attributes.remove(id);
    return true;
}

QList<DFileInfo::AttributeID> DLocalFileInfoPrivate::attributeIDList() const
{
    return attributes.keys();
}

bool DLocalFileInfoPrivate::exists() const
{
    const QUrl &url = q->uri();
    const QString &path = url.toString();

    GFile *gfile = g_file_new_for_path(path.toLocal8Bit().data());
    const bool exists = g_file_query_exists(gfile, nullptr);

    g_object_unref(gfile);
    return exists;
}

bool DLocalFileInfoPrivate::flush()
{
    auto it = attributes.constBegin();
    while (it != attributes.constEnd()) {
        DLocalHelper::setAttributeByGFileInfo(gfileinfo, it.key(), it.value());
        ++it;
    }
    return true;
}

uint16_t DLocalFileInfoPrivate::permissions(DFileInfo::Permission permission)
{
    if (permission == DFileInfo::Permission::NoPermission) {
        // 获取全部权限
        return permissionsAll();

    } else if (permission == DFileInfo::Permission::ExeUser
               || permission == DFileInfo::Permission::WriteUser
               || permission == DFileInfo::Permission::ReadUser) {
        // 获取当前user权限，需要调用gio接口
        return permissionsFromGio(permission);
    } else {
        // 获取单个权限
        return permissionsFromStat(permission);
    }
}

uint16_t DLocalFileInfoPrivate::permissionsAll()
{
    uint16_t retValue = 0x0000;
    // 获取系统默认权限
    const QUrl &url = q->uri();
    const char *path = url.toLocalFile().toLocal8Bit().data();

    struct stat buf;
    stat(path, &buf);

    if ((buf.st_mode & S_IXUSR) == S_IXUSR)
        retValue |= uint16_t(DFileInfo::Permission::ExeOwner);
    if ((buf.st_mode & S_IWUSR) == S_IWUSR)
        retValue |= uint16_t(DFileInfo::Permission::WriteOwner);
    if ((buf.st_mode & S_IRUSR) == S_IRUSR)
        retValue |= uint16_t(DFileInfo::Permission::ReadOwner);
    if ((buf.st_mode & S_IXGRP) == S_IXGRP)
        retValue |= uint16_t(DFileInfo::Permission::ExeGroup);
    if ((buf.st_mode & S_IWGRP) == S_IWGRP)
        retValue |= uint16_t(DFileInfo::Permission::WriteGroup);
    if ((buf.st_mode & S_IRGRP) == S_IRGRP)
        retValue |= uint16_t(DFileInfo::Permission::ReadGroup);
    if ((buf.st_mode & S_IXOTH) == S_IXOTH)
        retValue |= uint16_t(DFileInfo::Permission::ExeOther);
    if ((buf.st_mode & S_IWOTH) == S_IWOTH)
        retValue |= uint16_t(DFileInfo::Permission::WriteOther);
    if ((buf.st_mode & S_IROTH) == S_IROTH)
        retValue |= uint16_t(DFileInfo::Permission::ReadOther);

    if (gfileinfo) {
        if (attribute(DFileInfo::AttributeID::AccessCanExecute).toBool())
            retValue |= uint16_t(DFileInfo::Permission::ExeUser);
        if (attribute(DFileInfo::AttributeID::AccessCanWrite).toBool())
            retValue |= uint16_t(DFileInfo::Permission::WriteUser);
        if (attribute(DFileInfo::AttributeID::AccessCanRead).toBool())
            retValue |= uint16_t(DFileInfo::Permission::ReadUser);
    }
    return retValue;
}

uint16_t DLocalFileInfoPrivate::permissionsFromGio(DFileInfo::Permission permission)
{
    uint16_t retValue = 0x0000;
    if (gfileinfo) {
        if (permission == DFileInfo::Permission::ExeUser) {
            bool hasRight = attribute(DFileInfo::AttributeID::AccessCanExecute).toBool();
            retValue = uint16_t(hasRight ? DFileInfo::Permission::ExeUser : DFileInfo::Permission::NoPermission);
        } else if (permission == DFileInfo::Permission::WriteUser) {
            bool hasRight = attribute(DFileInfo::AttributeID::AccessCanWrite).toBool();
            retValue = uint16_t(hasRight ? DFileInfo::Permission::WriteUser : DFileInfo::Permission::NoPermission);
        } else if (permission == DFileInfo::Permission::ReadUser) {
            bool hasRight = attribute(DFileInfo::AttributeID::AccessCanRead).toBool();
            retValue = uint16_t(hasRight ? DFileInfo::Permission::ReadUser : DFileInfo::Permission::NoPermission);
        }
    }
    return retValue;
}

uint16_t DLocalFileInfoPrivate::permissionsFromStat(DFileInfo::Permission permission)
{
    const QUrl &url = q->uri();
    const char *path = url.toString().toLocal8Bit().data();

    struct stat buf;
    stat(path, &buf);

    switch (permission) {
    case DFileInfo::Permission::ExeOwner:
        if ((buf.st_mode & S_IXUSR) == S_IXUSR)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::WriteOwner:
        if ((buf.st_mode & S_IWUSR) == S_IWUSR)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::ReadOwner:
        if ((buf.st_mode & S_IRUSR) == S_IRUSR)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::ExeGroup:
        if ((buf.st_mode & S_IXGRP) == S_IXGRP)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::WriteGroup:
        if ((buf.st_mode & S_IWGRP) == S_IWGRP)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::ReadGroup:
        if ((buf.st_mode & S_IRGRP) == S_IRGRP)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::ExeOther:
        if ((buf.st_mode & S_IXOTH) == S_IXOTH)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::WriteOther:
        if ((buf.st_mode & S_IWOTH) == S_IWOTH)
            return uint16_t(permission);
        break;
    case DFileInfo::Permission::ReadOther:
        if ((buf.st_mode & S_IROTH) == S_IROTH)
            return uint16_t(permission);
        break;
    default:
        break;
    }
}
DLocalFileInfo::DLocalFileInfo(const QUrl &uri) : DFileInfo(uri)
    , d(new DLocalFileInfoPrivate(this))
{
    registerAttribute(std::bind(&DLocalFileInfo::attribute, this, std::placeholders::_1, std::placeholders::_2));
    registerSetAttribute(std::bind(&DLocalFileInfo::setAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerHasAttribute(std::bind(&DLocalFileInfo::hasAttribute, this, std::placeholders::_1));
    registerRemoveAttribute(std::bind(&DLocalFileInfo::removeAttribute, this, std::placeholders::_1));
    registerAttributeList(std::bind(&DLocalFileInfo::attributeIDList, this));
    registerExists(std::bind(&DLocalFileInfo::exists, this));
    registerFlush(std::bind(&DLocalFileInfo::flush, this));
    registerPermissions(std::bind(&DLocalFileInfo::permissions, this, std::placeholders::_1));

    d->init();
}

DLocalFileInfo::~DLocalFileInfo()
{

}

QVariant DLocalFileInfo::attribute(DFileInfo::AttributeID id, bool *success /*= nullptr */)
{
    return d->attribute(id, success);
}

bool DLocalFileInfo::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    return d->setAttribute(id, value);
}

bool DLocalFileInfo::hasAttribute(DFileInfo::AttributeID id)
{
    return d->hasAttribute(id);
}

bool DLocalFileInfo::removeAttribute(DFileInfo::AttributeID id)
{
    return d->removeAttribute(id);
}

QList<DFileInfo::AttributeID> DLocalFileInfo::attributeIDList() const
{
    return d->attributeIDList();
}

bool DLocalFileInfo::exists() const
{
    return d->exists();
}

bool DLocalFileInfo::flush()
{
    return d->flush();
}

uint16_t DLocalFileInfo::permissions(DFileInfo::Permission permission)
{
    return d->permissions(permission);
}
