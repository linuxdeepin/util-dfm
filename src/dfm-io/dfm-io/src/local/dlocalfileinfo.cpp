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
#include <QPointer>
#include <QDebug>

USING_IO_NAMESPACE

DLocalFileInfoPrivate::DLocalFileInfoPrivate(DLocalFileInfo *q)
    : q(q)
{
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kStandardIsHidden);
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

void DLocalFileInfoPrivate::initNormal()
{
    if (this->gfile)
        return;

    const QUrl &url = q->uri();
    const QString &urlStr = url.toString();

    this->gfile = g_file_new_for_uri(urlStr.toLocal8Bit().data());
}

bool DLocalFileInfoPrivate::queryInfoSync()
{
    if (!infoReseted && this->gfileinfo) {
        initFinished = true;
        return true;
    }

    const char *attributes = q->queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = q->queryInfoFlag();

    g_autoptr(GError) gerror = nullptr;
    GFileInfo *gfileinfo = g_file_query_info(gfile, attributes, GFileQueryInfoFlags(flag), nullptr, &gerror);
    if (gerror)
        setErrorFromGError(gerror);
    if (!gfileinfo)
        return false;

    if (this->gfileinfo) {
        g_object_unref(this->gfileinfo);
        this->gfileinfo = nullptr;
    }
    this->gfileinfo = gfileinfo;
    initFinished = true;

    return true;
}

typedef struct
{
    DFileInfo::QueryInfoAsyncCallback callback;
    gpointer user_data;
    QPointer<DLocalFileInfoPrivate> me;
} queryInfoAsyncOp;

void queryInfoAsyncCallback(GObject *source_object,
                            GAsyncResult *res,
                            gpointer user_data)
{
    queryInfoAsyncOp *data = static_cast<queryInfoAsyncOp *>(user_data);
    GFile *file = (GFile *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    GFileInfo *fileinfo = g_file_query_info_finish(file, res, &gerror);

    if (data->me) {
        data->me->gfileinfo = fileinfo;
        data->me->initFinished = true;
    }

    if (data->callback)
        data->callback(fileinfo ? true : false, data->user_data);
}

void DLocalFileInfoPrivate::queryInfoAsync(int ioPriority, DFileInfo::QueryInfoAsyncCallback func, void *userData)
{
    if (!infoReseted && this->gfileinfo) {
        initFinished = true;

        if (func)
            func(true, userData);
        return;
    }

    const char *attributes = q->queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = q->queryInfoFlag();

    queryInfoAsyncOp *dataOp = g_new0(queryInfoAsyncOp, 1);
    dataOp->callback = func;
    dataOp->user_data = userData;
    dataOp->me = this;

    g_file_query_info_async(this->gfile, attributes, GFileQueryInfoFlags(flag), ioPriority, nullptr, queryInfoAsyncCallback, dataOp);
}

QVariant DLocalFileInfoPrivate::attribute(DFileInfo::AttributeID id, bool *success)
{
    if (!initFinished) {
        bool succ = queryInfoSync();
        if (!succ)
            return QVariant();
    }

    QVariant retValue;
    if (attributesCache.count(id) == 0) {
        if (id > DFileInfo::AttributeID::kCustomStart) {
            const QString &path = q->uri().path();
            retValue = DLocalHelper::customAttributeFromPathAndInfo(path, gfileinfo, id);
        } else {
            if (gfileinfo) {
                DFMIOErrorCode errorCode(DFM_IO_ERROR_NONE);
                if (!attributesRealizationSelf.contains(id)) {
                    retValue = DLocalHelper::attributeFromGFileInfo(gfileinfo, id, errorCode);
                    if (errorCode != DFM_IO_ERROR_NONE)
                        error.setCode(errorCode);
                } else {
                    retValue = attributesBySelf(id);
                }
            }
        }
        if (retValue.isValid())
            cacheAttribute(id, retValue);

        if (success)
            *success = retValue.isValid();
        if (!retValue.isValid())
            retValue = std::get<1>(DFileInfo::attributeInfoMap.at(id));
        return retValue;
    }
    if (success)
        *success = true;
    return attributesCache.value(id);
}

bool DLocalFileInfoPrivate::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    cacheAttribute(id, value);
    DLocalHelper::setAttributeByGFileInfo(gfileinfo, id, value);
    return true;
}

bool DLocalFileInfoPrivate::hasAttribute(DFileInfo::AttributeID id)
{
    if (attributesCache.count(id) > 0)
        return true;

    if (!initFinished) {
        bool succ = queryInfoSync();
        if (!succ)
            return false;
    }

    if (gfileinfo) {
        const std::string &key = DLocalHelper::attributeStringById(id);
        return g_file_info_has_attribute(gfileinfo, key.c_str());
    }

    return false;
}

bool DLocalFileInfoPrivate::removeAttribute(DFileInfo::AttributeID id)
{
    if (attributesCache.count(id) > 0)
        attributesCache.remove(id);
    return true;
}

bool DLocalFileInfoPrivate::cacheAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    if (attributesCache.count(id) > 0)
        attributesCache.remove(id);

    attributesCache.insert(id, value);
    return true;
}

QVariant DLocalFileInfoPrivate::attributesBySelf(DFileInfo::AttributeID id)
{
    QVariant retValue;
    switch (id) {
    case DFileInfo::AttributeID::kStandardIsHidden: {
        retValue = DLocalHelper::fileIsHidden(q->sharedFromThis(), {});
        break;
    }
    default:
        return retValue;
    }
    return retValue;
}

QList<DFileInfo::AttributeID> DLocalFileInfoPrivate::attributeIDList() const
{
    return attributesCache.keys();
}

bool DLocalFileInfoPrivate::exists() const
{
    /*const QUrl &url = q->uri();
    const QString &uri = url.toString();

    g_autoptr(GFile) gfile = g_file_new_for_uri(uri.toLocal8Bit().data());

    return g_file_query_file_type(gfile, G_FILE_QUERY_INFO_NONE, nullptr) != G_FILE_TYPE_UNKNOWN;*/
    // g_file_query_file_type will block io, use g_file_info_get_file_type instead
    return g_file_info_get_file_type(gfileinfo) != G_FILE_TYPE_UNKNOWN;
}

bool DLocalFileInfoPrivate::refresh()
{
    infoReseted = true;
    bool ret = queryInfoSync();
    infoReseted = false;

    return ret;
}

bool DLocalFileInfoPrivate::clearCache()
{
    attributesCache.clear();
    return true;
}

DFile::Permissions DLocalFileInfoPrivate::permissions()
{
    const QUrl &url = q->uri();

    QSharedPointer<DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_NOT_INITIALIZED);
        return DFile::Permission::kNoPermission;
    }

    QSharedPointer<DFile> dfile = factory->createFile();

    if (!dfile) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_NOT_INITIALIZED);
        return DFile::Permission::kNoPermission;
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
    if (!initFinished) {
        bool succ = queryInfoSync();
        if (!succ)
            return QVariant();
    }

    if (!gfileinfo)
        return QVariant();

    switch (type) {
    case DFileInfo::DFileAttributeType::kTypeString: {
        const char *ret = g_file_info_get_attribute_string(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeByteString: {
        const char *ret = g_file_info_get_attribute_byte_string(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeBool: {
        bool ret = g_file_info_get_attribute_boolean(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeUInt32: {
        uint32_t ret = g_file_info_get_attribute_uint32(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeInt32: {
        int32_t ret = g_file_info_get_attribute_int32(gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeUInt64: {
        uint64_t ret = g_file_info_get_attribute_uint64(gfileinfo, key);
        return QVariant(qulonglong(ret));
    }
    case DFileInfo::DFileAttributeType::kTypeInt64: {
        int64_t ret = g_file_info_get_attribute_int64(gfileinfo, key);
        return QVariant(qulonglong(ret));
    }
    case DFileInfo::DFileAttributeType::kTypeStringV: {
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

void DLocalFileInfoPrivate::freeCancellable(GCancellable *gcancellable)
{
}

DLocalFileInfo::DLocalFileInfo(const QUrl &uri,
                               const char *attributes /*= "*"*/,
                               const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/)
    : DFileInfo(uri, attributes, flag), d(new DLocalFileInfoPrivate(this))
{
    registerAttribute(std::bind(&DLocalFileInfo::attribute, this, std::placeholders::_1, std::placeholders::_2));
    registerSetAttribute(std::bind(&DLocalFileInfo::setAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerHasAttribute(std::bind(&DLocalFileInfo::hasAttribute, this, std::placeholders::_1));
    registerRemoveAttribute(std::bind(&DLocalFileInfo::removeAttribute, this, std::placeholders::_1));
    registerAttributeList(std::bind(&DLocalFileInfo::attributeIDList, this));
    registerExists(std::bind(&DLocalFileInfo::exists, this));
    registerRefresh(std::bind(&DLocalFileInfo::refresh, this));
    registerClearCache(std::bind(&DLocalFileInfo::clearCache, this));
    registerPermissions(std::bind(&DLocalFileInfo::permissions, this));
    registerSetCustomAttribute(std::bind(&DLocalFileInfo::setCustomAttribute, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    registerCustomAttribute(std::bind(&DLocalFileInfo::customAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerLastError(std::bind(&DLocalFileInfo::lastError, this));
    registerQueryInfoAsync(bind_field(this, &DLocalFileInfo::queryInfoAsync));

    d->initNormal();
}

DLocalFileInfo::DLocalFileInfo(const QUrl &uri, void *fileInfo, const char *attributes, const DFileInfo::FileQueryInfoFlags flag)
    : DLocalFileInfo(uri, attributes, flag)
{
    d->gfileinfo = static_cast<GFileInfo *>(fileInfo);
}

DLocalFileInfo::~DLocalFileInfo()
{
}

void DLocalFileInfo::queryInfoAsync(int ioPriority, DFileInfo::QueryInfoAsyncCallback func, void *userData) const
{
    d->queryInfoAsync(ioPriority, func, userData);
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

bool DLocalFileInfo::refresh()
{
    return d->refresh();
}

bool DLocalFileInfo::clearCache()
{
    return d->clearCache();
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
