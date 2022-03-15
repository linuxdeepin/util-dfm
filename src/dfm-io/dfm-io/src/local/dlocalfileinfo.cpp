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
#include "core/diofactory.h"
#include "dfmio_register.h"

#include <QVariant>
#include <QDebug>

USING_IO_NAMESPACE

DLocalFileInfoPrivate::DLocalFileInfoPrivate(DLocalFileInfo *q)
    : q(q)
{
}

DLocalFileInfoPrivate::~DLocalFileInfoPrivate()
{
    if (gfileinfo) {
        g_object_unref(gfileinfo);
        gfileinfo = nullptr;
    }
    if (gfile) {
        g_object_unref(gfile);
        gfile = nullptr;
    }
}

bool DLocalFileInfoPrivate::init()
{
    const QUrl &url = q->uri();
    const DFileInfo::FileQueryInfoFlags flag = q->fileQueryInfoFlag();
    const QString &path = url.toString();

    GFile *gfile = g_file_new_for_uri(path.toLocal8Bit().data());

    GError *gerror = nullptr;

    GFileInfo *gfileinfo = g_file_query_info(gfile, "*", GFileQueryInfoFlags(flag), nullptr, &gerror);

    if (gerror) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
    }

    if (!gfileinfo)
        return false;

    this->gfile = gfile;
    this->gfileinfo = gfileinfo;
    return true;
}

QVariant DLocalFileInfoPrivate::attribute(DFileInfo::AttributeID id, bool *success)
{
    QVariant retValue;
    if (attributes.count(id) == 0) {
        if (id > DFileInfo::AttributeID::CustomStart) {
            const QString &path = q->uri().path();
            retValue = DLocalHelper::customAttributeFromPath(path, id);
        } else {
            if (gfileinfo) {
                DFMIOErrorCode errorCode(DFM_IO_ERROR_NONE);
                retValue = DLocalHelper::attributeFromGFileInfo(gfileinfo, id, errorCode);
                if (errorCode != DFM_IO_ERROR_NONE)
                    error.setCode(errorCode);
            }
        }
        if (retValue.isValid())
            setAttribute(id, retValue);

        if (success)
            *success = retValue.isValid();
        if (!retValue.isValid())
            retValue = std::get<1>(DFileInfo::attributeInfoMap.at(id));
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
        DFMIOErrorCode errorCode(DFM_IO_ERROR_NONE);
        const QVariant &value = DLocalHelper::attributeFromGFileInfo(gfileinfo, id, errorCode);
        if (errorCode != DFM_IO_ERROR_NONE)
            error.setCode(errorCode);
        if (value.isValid()) {
            setAttribute(id, value);
            return true;
        }
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
    const QString &uri = url.toString();

    g_autoptr(GFile) gfile = g_file_new_for_uri(uri.toLocal8Bit().data());
    const bool exists = g_file_query_exists(gfile, nullptr);

    return exists;
}

bool DLocalFileInfoPrivate::flush()
{
    bool ret = true;
    auto it = attributes.constBegin();
    while (it != attributes.constEnd()) {
        g_autoptr(GError) gerror = nullptr;
        bool succ = DLocalHelper::setAttributeByGFile(gfile, it.key(), it.value(), &gerror);
        if (!succ)
            ret = false;
        if (gerror)
            setErrorFromGError(gerror);
        ++it;
    }
    return ret;
}

DFile::Permissions DLocalFileInfoPrivate::permissions()
{
    const QUrl &url = q->uri();

    QSharedPointer<DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_NOT_INITIALIZED);
        return DFile::Permission::NoPermission;
    }

    QSharedPointer<DFile> dfile = factory->createFile();

    if (!dfile) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_NOT_INITIALIZED);
        return DFile::Permission::NoPermission;
    }

    return dfile->permissions();
}

bool DLocalFileInfoPrivate::setCustomAttribute(const char *key, const DFileInfo::DFileAttributeType type, const void *value, const DFileInfo::FileQueryInfoFlags flag)
{
    if (gfile) {
        g_autoptr(GError) gerror = nullptr;
        bool ret = g_file_set_attribute(gfile, key, GFileAttributeType(type), (gpointer)(value), GFileQueryInfoFlags(flag), nullptr, &gerror);

        if (gerror)
            setErrorFromGError(gerror);
        return ret;
    }
    return false;
}

QVariant DLocalFileInfoPrivate::customAttribute(const char *key, const DFileInfo::DFileAttributeType type)
{
    switch (type) {
    case DFileInfo::DFileAttributeType::TypeString: {
        const char *ret = g_file_info_get_attribute_string(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::TypeByteString: {
        const char *ret = g_file_info_get_attribute_byte_string(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::TypeBool: {
        bool ret = g_file_info_get_attribute_boolean(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::TypeUInt32: {
        uint32_t ret = g_file_info_get_attribute_uint32(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::TypeInt32: {
        int32_t ret = g_file_info_get_attribute_int32(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::TypeUInt64: {
        uint64_t ret = g_file_info_get_attribute_uint64(gfileinfo, key);
        return QVariant(qulonglong(ret));
    }
    case DFileInfo::DFileAttributeType::TypeInt64: {
        int64_t ret = g_file_info_get_attribute_int64(gfileinfo, key);
        return QVariant(qulonglong(ret));
    }
    case DFileInfo::DFileAttributeType::TypeStringV: {
        char **ret = g_file_info_get_attribute_stringv(gfileinfo, key);
        QStringList retValue;
        for (int i = 0; ret && ret[i]; ++i) {
            retValue.append(QString::fromLocal8Bit(ret[i]));
        }
        return retValue;
    }
    default:
        return QVariant();
    }
}

DFMIOError DLocalFileInfoPrivate::lastError()
{
    return error;
}

void DLocalFileInfoPrivate::setErrorFromGError(GError *gerror)
{
    error.setCode(DFMIOErrorCode(gerror->code));
}

DLocalFileInfo::DLocalFileInfo(const QUrl &uri)
    : DFileInfo(uri), d(new DLocalFileInfoPrivate(this))
{
    registerAttribute(std::bind(&DLocalFileInfo::attribute, this, std::placeholders::_1, std::placeholders::_2));
    registerSetAttribute(std::bind(&DLocalFileInfo::setAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerHasAttribute(std::bind(&DLocalFileInfo::hasAttribute, this, std::placeholders::_1));
    registerRemoveAttribute(std::bind(&DLocalFileInfo::removeAttribute, this, std::placeholders::_1));
    registerAttributeList(std::bind(&DLocalFileInfo::attributeIDList, this));
    registerExists(std::bind(&DLocalFileInfo::exists, this));
    registerFlush(std::bind(&DLocalFileInfo::flush, this));
    registerPermissions(std::bind(&DLocalFileInfo::permissions, this));
    registerSetCustomAttribute(std::bind(&DLocalFileInfo::setCustomAttribute, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    registerCustomAttribute(std::bind(&DLocalFileInfo::customAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerLastError(std::bind(&DLocalFileInfo::lastError, this));

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

DFile::Permissions DLocalFileInfo::permissions()
{
    return d->permissions();
}

bool DLocalFileInfo::setCustomAttribute(const char *key, const DFileInfo::DFileAttributeType type, const void *value, const DFileInfo::FileQueryInfoFlags flag)
{
    return d->setCustomAttribute(key, type, value, flag);
}

QVariant DLocalFileInfo::customAttribute(const char *key, const DFileInfo::DFileAttributeType type)
{
    return d->customAttribute(key, type);
}

DFMIOError DLocalFileInfo::lastError() const
{
    return d->lastError();
}
