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
    GFileInfo *fileinfo = g_file_query_info(gfile, attributes, GFileQueryInfoFlags(flag), nullptr, &gerror);
    if (gerror)
        setErrorFromGError(gerror);
    if (!fileinfo)
        return false;

    if (this->gfileinfo) {
        g_object_unref(this->gfileinfo);
        this->gfileinfo = nullptr;
    }
    this->gfileinfo = fileinfo;
    initFinished = true;

    return true;
}

void DLocalFileInfoPrivate::queryInfoAsync(int ioPriority, DFileInfo::InitQuerierAsyncCallback func, void *userData)
{
    if (!infoReseted && this->gfileinfo) {
        initFinished = true;

        if (func)
            func(true, userData);
        return;
    }

    const char *attributes = q->queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = q->queryInfoFlag();

    QueryInfoAsyncOp *dataOp = g_new0(QueryInfoAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;
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

    if (success)
        *success = retValue.isValid();

    if (!retValue.isValid())
        retValue = std::get<1>(DFileInfo::attributeInfoMap.at(id));

    return retValue;
}

typedef struct
{
    DFileInfo::AttributeAsyncCallback callback;
    gpointer user_data;
    DFileInfo::AttributeID id;
    QPointer<DLocalFileInfoPrivate> me;
} QueryFileInfoFromAttributeOp;

void queryFileInfoFromAttributeCallback(bool ok, void *userData)
{
    QueryFileInfoFromAttributeOp *dataOp = static_cast<QueryFileInfoFromAttributeOp *>(userData);
    if (!dataOp)
        return;

    if (dataOp->callback) {
        if (ok) {
            bool success = false;
            const QVariant &value = dataOp->me->attribute(dataOp->id, &success);
            dataOp->callback(success, dataOp->user_data, value);
        } else {
            dataOp->callback(false, dataOp->user_data, QVariant());
        }
    }

    dataOp->callback = nullptr;
    dataOp->user_data = nullptr;
    dataOp->me = nullptr;
    g_free(dataOp);
}

void DLocalFileInfoPrivate::attributeAsync(DFileInfo::AttributeID id, bool *success, int ioPriority, DFileInfo::AttributeAsyncCallback func, void *userData)
{
    if (!initFinished) {
        // query async
        QueryFileInfoFromAttributeOp *dataOp = g_new0(QueryFileInfoFromAttributeOp, 1);
        dataOp->callback = func;
        dataOp->user_data = userData;
        dataOp->id = id;
        dataOp->me = this;

        queryInfoAsync(ioPriority, queryFileInfoFromAttributeCallback, dataOp);
        return;
    }

    const QVariant &value = attribute(id, success);
    if (func)
        func(success, userData, value);
}

bool DLocalFileInfoPrivate::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    return DLocalHelper::setAttributeByGFileInfo(gfileinfo, id, value);
}

bool DLocalFileInfoPrivate::hasAttribute(DFileInfo::AttributeID id)
{
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

void DLocalFileInfoPrivate::freeQueryInfoAsyncOp(QueryInfoAsyncOp *op)
{
    op->callback = nullptr;
    op->userData = nullptr;
    op->me = nullptr;
    g_free(op);
}

bool DLocalFileInfoPrivate::exists() const
{
    if (!gfileinfo)
        return false;
    return g_file_info_get_file_type(gfileinfo) != G_FILE_TYPE_UNKNOWN;
}

bool DLocalFileInfoPrivate::refresh()
{
    infoReseted = true;
    bool ret = queryInfoSync();
    infoReseted = false;

    return ret;
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

void DLocalFileInfoPrivate::queryInfoAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    QueryInfoAsyncOp *data = static_cast<QueryInfoAsyncOp *>(userData);
    if (!data)
        return;

    GFile *file = G_FILE(sourceObject);
    if (!file) {
        freeQueryInfoAsyncOp(data);
        return;
    }

    g_autoptr(GError) gerror = nullptr;
    GFileInfo *fileinfo = g_file_query_info_finish(file, res, &gerror);

    if (gerror) {
        data->me->setErrorFromGError(gerror);
        freeQueryInfoAsyncOp(data);
        return;
    }

    if (data->me) {
        data->me->gfileinfo = fileinfo;
        data->me->initFinished = true;
    }

    if (data->callback)
        data->callback(fileinfo ? true : false, data->userData);

    freeQueryInfoAsyncOp(data);
}

DLocalFileInfo::DLocalFileInfo(const QUrl &uri,
                               const char *attributes /*= "*"*/,
                               const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/)
    : DFileInfo(uri, attributes, flag), d(new DLocalFileInfoPrivate(this))
{
    registerInitQuerier(std::bind(&DLocalFileInfo::initQuerier, this));
    registerInitQuerierAsync(std::bind(&DLocalFileInfo::initQuerierAsync, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    registerAttribute(std::bind(&DLocalFileInfo::attribute, this, std::placeholders::_1, std::placeholders::_2));
    registerAttributeAsync(bind_field(this, &DLocalFileInfo::attributeAsync));
    registerSetAttribute(std::bind(&DLocalFileInfo::setAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerHasAttribute(std::bind(&DLocalFileInfo::hasAttribute, this, std::placeholders::_1));
    registerExists(std::bind(&DLocalFileInfo::exists, this));
    registerRefresh(std::bind(&DLocalFileInfo::refresh, this));
    registerPermissions(std::bind(&DLocalFileInfo::permissions, this));
    registerSetCustomAttribute(std::bind(&DLocalFileInfo::setCustomAttribute, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    registerCustomAttribute(std::bind(&DLocalFileInfo::customAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerLastError(std::bind(&DLocalFileInfo::lastError, this));

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

bool DLocalFileInfo::initQuerier()
{
    return d->queryInfoSync();
}

void DLocalFileInfo::initQuerierAsync(int ioPriority, DFileInfo::InitQuerierAsyncCallback func, void *userData)
{
    d->queryInfoAsync(ioPriority, func, userData);
}

QVariant DLocalFileInfo::attribute(DFileInfo::AttributeID id, bool *success /*= nullptr */) const
{
    return d->attribute(id, success);
}

void DLocalFileInfo::attributeAsync(DFileInfo::AttributeID id, bool *success, int ioPriority, DFileInfo::AttributeAsyncCallback func, void *userData) const
{
    d->attributeAsync(id, success, ioPriority, func, userData);
}

bool DLocalFileInfo::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    return d->setAttribute(id, value);
}

bool DLocalFileInfo::hasAttribute(DFileInfo::AttributeID id) const
{
    return d->hasAttribute(id);
}

bool DLocalFileInfo::exists() const
{
    return d->exists();
}

bool DLocalFileInfo::refresh()
{
    return d->refresh();
}

DFile::Permissions DLocalFileInfo::permissions() const
{
    return d->permissions();
}

bool DLocalFileInfo::setCustomAttribute(const char *key, const DFileInfo::DFileAttributeType type, const void *value, const DFileInfo::FileQueryInfoFlags flag)
{
    return d->setCustomAttribute(key, type, value, flag);
}

QVariant DLocalFileInfo::customAttribute(const char *key, const DFileInfo::DFileAttributeType type) const
{
    return d->customAttribute(key, type);
}

DFMIOError DLocalFileInfo::lastError() const
{
    return d->lastError();
}
