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
#include "core/dfilefuture.h"
#include "dfmio_register.h"

#include <QVariant>
#include <QPointer>
#include <QTimer>
#include <QDebug>

#include <sys/stat.h>
#include <fcntl.h>

USING_IO_NAMESPACE

DLocalFileInfoPrivate::DLocalFileInfoPrivate(DLocalFileInfo *q)
    : q(q)
{
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kStandardIsHidden);
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kTimeCreated);
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kTimeCreatedUsec);
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kTimeModified);
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kTimeModifiedUsec);
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kTimeAccess);
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kTimeAccessUsec);

    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardName);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardDisplayName);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardEditName);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardCopyName);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardSuffix);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardCompleteSuffix);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardFilePath);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardParentPath);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardBaseName);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardFileName);
    attributesNoBlockIO.push_back(DFileInfo::AttributeID::kStandardCompleteBaseName);
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
        if (!succ) {
            if (!attributesNoBlockIO.contains(id))
                return QVariant();
            else
                return attributesFromUrl(id);
        }
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

DFileFuture *DLocalFileInfoPrivate::initQuerierAsync(int ioPriority, QObject *parent)
{
    const char *attributes = q->queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = q->queryInfoFlag();

    DFileFuture *future = new DFileFuture(parent);
    QueryInfoAsyncOp2 *dataOp = g_new0(QueryInfoAsyncOp2, 1);
    dataOp->future = future;
    dataOp->me = this;

    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_file_query_info_async(this->gfile, attributes, GFileQueryInfoFlags(flag), ioPriority, cancellable, queryInfoAsyncCallback2, dataOp);
    return future;
}

DFileFuture *DLocalFileInfoPrivate::attributeAsync(DFileInfo::AttributeID id, int ioPriority, QObject *parent)
{
    DFileFuture *futureRet = new DFileFuture(parent);
    if (!initFinished) {
        DFileFuture *future = this->initQuerierAsync(ioPriority, nullptr);
        connect(future, &DFileFuture::finished, this, [=]() {
            if (!future->hasError()) {
                futureRet->infoAttribute(id, attribute(id));
                futureRet->finished();
            }
            future->deleteLater();
        });
    }
    QTimer::singleShot(10, this, [=]() {
        futureRet->infoAttribute(id, attribute(id));
        futureRet->finished();
    });
    return futureRet;
}

DFileFuture *DLocalFileInfoPrivate::attributeAsync(const QByteArray &key, const DFileInfo::DFileAttributeType type, int ioPriority, QObject *parent)
{
    DFileFuture *futureRet = new DFileFuture(parent);
    if (!initFinished) {
        DFileFuture *future = this->initQuerierAsync(ioPriority, nullptr);
        connect(future, &DFileFuture::finished, this, [=]() {
            if (!future->hasError()) {
                futureRet->infoAttribute(key, customAttribute(key, type));
                futureRet->finished();
            }
            future->deleteLater();
        });
    }
    QTimer::singleShot(10, this, [=]() {
        futureRet->infoAttribute(key, customAttribute(key, type));
        futureRet->finished();
    });
    return futureRet;
}

DFileFuture *DLocalFileInfoPrivate::existsAsync(int ioPriority, QObject *parent)
{
    DFileFuture *futureRet = new DFileFuture(parent);
    if (!initFinished) {
        DFileFuture *future = this->initQuerierAsync(ioPriority, nullptr);
        connect(future, &DFileFuture::finished, this, [=]() {
            if (!future->hasError()) {
                const bool exists = this->exists();
                futureRet->infoExists(exists);
                futureRet->finished();
            }
            future->deleteLater();
        });
    }
    QTimer::singleShot(10, this, [=]() {
        const bool exists = this->exists();
        futureRet->infoExists(exists);
        futureRet->finished();
    });
    return futureRet;
}

DFileFuture *DLocalFileInfoPrivate::refreshAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = this->initQuerierAsync(ioPriority, parent);
    connect(future, &DFileFuture::finished, this, [=]() {
        future->finished();
    });
    return future;
}

DFileFuture *DLocalFileInfoPrivate::permissionsAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = this->initQuerierAsync(ioPriority, parent);
    connect(future, &DFileFuture::finished, this, [=]() {
        future->infoPermissions(this->permissions());
        future->finished();
    });
    return future;
}

bool DLocalFileInfoPrivate::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    // discard
    return false;
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
        if (key.empty())
            return false;
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
    case DFileInfo::AttributeID::kTimeCreated: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint64_t ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret == 0) {
                return statxBuffer.stx_btime.tv_sec > 0
                        ? quint64(statxBuffer.stx_btime.tv_sec)
                        : quint64(statxBuffer.stx_ctime.tv_sec);
            }
        }
        return qulonglong(ret);
    }
    case DFileInfo::AttributeID::kTimeCreatedUsec: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint32_t ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret == 0) {
                return statxBuffer.stx_btime.tv_nsec > 0
                        ? statxBuffer.stx_btime.tv_nsec / 1000000
                        : statxBuffer.stx_ctime.tv_nsec / 1000000;
            }
        }
        return QVariant(ret);
    }
    case DFileInfo::AttributeID::kTimeModified: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint64_t ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret == 0) {
                return statxBuffer.stx_mtime.tv_sec > 0
                        ? quint64(statxBuffer.stx_mtime.tv_sec)
                        : quint64(statxBuffer.stx_ctime.tv_sec);
            }
        }
        return qulonglong(ret);
    }
    case DFileInfo::AttributeID::kTimeModifiedUsec: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint32_t ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret == 0) {
                return statxBuffer.stx_mtime.tv_nsec > 0
                        ? statxBuffer.stx_mtime.tv_nsec / 1000000
                        : statxBuffer.stx_ctime.tv_nsec / 1000000;
            }
        }
        return QVariant(ret);
    }
    case DFileInfo::AttributeID::kTimeAccess: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint64_t ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret == 0) {
                return statxBuffer.stx_atime.tv_sec > 0
                        ? quint64(statxBuffer.stx_atime.tv_sec)
                        : quint64(statxBuffer.stx_ctime.tv_sec);
            }
        }
        return qulonglong(ret);
    }
    case DFileInfo::AttributeID::kTimeAccessUsec: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint32_t ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret == 0) {
                return statxBuffer.stx_atime.tv_nsec > 0
                        ? statxBuffer.stx_atime.tv_nsec / 1000000
                        : statxBuffer.stx_ctime.tv_nsec / 1000000;
            }
        }
        return QVariant(ret);
    }
    default:
        return retValue;
    }
    return retValue;
}

QVariant DLocalFileInfoPrivate::attributesFromUrl(DFileInfo::AttributeID id)
{
    if (!attributesNoBlockIO.contains(id))
        return QVariant();

    QVariant retValue;
    switch (id) {
    case DFileInfo::AttributeID::kStandardName:
    case DFileInfo::AttributeID::kStandardDisplayName:
    case DFileInfo::AttributeID::kStandardEditName:
    case DFileInfo::AttributeID::kStandardCopyName:
    case DFileInfo::AttributeID::kStandardFileName: {
        g_autofree gchar *name = g_path_get_basename(q->uri().toString().toStdString().c_str());
        if (name != nullptr)
            return QString::fromLocal8Bit(name);
        return "";
    }
    case DFileInfo::AttributeID::kStandardSuffix: {
        // path
        const QString &fullName = attributesFromUrl(DFileInfo::AttributeID::kStandardName).toString();

        int pos2 = fullName.lastIndexOf(".");
        if (pos2 == -1)
            return "";
        else
            return fullName.mid(pos2 + 1);
    }
    case DFileInfo::AttributeID::kStandardCompleteSuffix: {
        const QString &fullName = attributesFromUrl(DFileInfo::AttributeID::kStandardName).toString();

        int pos2 = fullName.indexOf(".");
        if (pos2 == -1)
            return "";
        else
            return fullName.mid(pos2 + 1);
    }
    case DFileInfo::AttributeID::kStandardFilePath: {
        g_autofree gchar *name = g_path_get_dirname(q->uri().toString().toStdString().c_str());
        if (name != nullptr)
            return QString::fromLocal8Bit(name);
        return "";
    }
    case DFileInfo::AttributeID::kStandardParentPath: {
        g_autoptr(GFile) file = g_file_new_for_path(q->uri().path().toStdString().c_str());
        g_autoptr(GFile) fileParent = g_file_get_parent(file);   // no blocking I/O

        g_autofree gchar *gpath = g_file_get_path(fileParent);   // no blocking I/O
        if (gpath != nullptr)
            return QString::fromLocal8Bit(gpath);
        return "";
    }
    case DFileInfo::AttributeID::kStandardBaseName: {
        const QString &fullName = attributesFromUrl(DFileInfo::AttributeID::kStandardName).toString();

        int pos2 = fullName.indexOf(".");
        if (pos2 == -1)
            return fullName;
        else
            return fullName.left(pos2);
    }
    case DFileInfo::AttributeID::kStandardCompleteBaseName: {
        const QString &fullName = attributesFromUrl(DFileInfo::AttributeID::kStandardName).toString();

        int pos2 = fullName.lastIndexOf(".");
        if (pos2 == -1)
            return fullName;
        else
            return fullName.left(pos2);
    }
    default:
        break;
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

void DLocalFileInfoPrivate::freeQueryInfoAsyncOp2(DLocalFileInfoPrivate::QueryInfoAsyncOp2 *op)
{
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
    DFile::Permissions retValue = DFile::Permission::kNoPermission;

    if (!initFinished) {
        bool succ = queryInfoSync();
        if (!succ)
            return retValue;
    }

    const QVariant &value = this->attribute(DFileInfo::AttributeID::kUnixMode);
    if (!value.isValid())
        return retValue;
    const uint32_t stMode = value.toUInt();

    if ((stMode & S_IXUSR) == S_IXUSR) {
        retValue |= DFile::Permission::kExeOwner;
        retValue |= DFile::Permission::kExeUser;
    }
    if ((stMode & S_IWUSR) == S_IWUSR) {
        retValue |= DFile::Permission::kWriteOwner;
        retValue |= DFile::Permission::kWriteUser;
    }
    if ((stMode & S_IRUSR) == S_IRUSR) {
        retValue |= DFile::Permission::kReadOwner;
        retValue |= DFile::Permission::kReadUser;
    }

    if ((stMode & S_IXGRP) == S_IXGRP)
        retValue |= DFile::Permission::kExeGroup;
    if ((stMode & S_IWGRP) == S_IWGRP)
        retValue |= DFile::Permission::kWriteGroup;
    if ((stMode & S_IRGRP) == S_IRGRP)
        retValue |= DFile::Permission::kReadGroup;

    if ((stMode & S_IXOTH) == S_IXOTH)
        retValue |= DFile::Permission::kExeOther;
    if ((stMode & S_IWOTH) == S_IWOTH)
        retValue |= DFile::Permission::kWriteOther;
    if ((stMode & S_IROTH) == S_IROTH)
        retValue |= DFile::Permission::kReadOther;

    return retValue;
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

void DLocalFileInfoPrivate::queryInfoAsyncCallback2(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    QueryInfoAsyncOp2 *data = static_cast<QueryInfoAsyncOp2 *>(userData);
    if (!data)
        return;

    DFileFuture *future = data->future;
    if (!future) {
        freeQueryInfoAsyncOp2(data);
        return;
    }

    GFile *file = G_FILE(sourceObject);
    if (!file) {
        freeQueryInfoAsyncOp2(data);
        return;
    }

    g_autoptr(GError) gerror = nullptr;
    GFileInfo *fileinfo = g_file_query_info_finish(file, res, &gerror);

    if (gerror) {
        data->me->setErrorFromGError(gerror);
        freeQueryInfoAsyncOp2(data);
        return;
    }

    if (data->me) {
        data->me->gfileinfo = fileinfo;
        data->me->initFinished = true;

        future->finished();
    }

    freeQueryInfoAsyncOp2(data);
}

DLocalFileInfo::DLocalFileInfo(const QUrl &uri,
                               const char *attributes /*= "*"*/,
                               const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/)
    : DFileInfo(uri, attributes, flag), d(new DLocalFileInfoPrivate(this))
{
    using namespace std::placeholders;
    registerInitQuerier(std::bind(&DLocalFileInfo::initQuerier, this));
    using funcinitQuerierAsync = void (DLocalFileInfo::*)(int, InitQuerierAsyncCallback, void *);
    registerInitQuerierAsync(std::bind((funcinitQuerierAsync)&DLocalFileInfo::initQuerierAsync, this, _1, _2, _3));
    registerAttribute(std::bind(&DLocalFileInfo::attribute, this, _1, _2));
    using funcAttributeAsync = void (DLocalFileInfo::*)(DFileInfo::AttributeID, bool *, int, AttributeAsyncCallback, void *) const;
    registerAttributeAsync(std::bind((funcAttributeAsync)&DLocalFileInfo::attributeAsync, this, _1, _2, _3, _4, _5));
    registerSetAttribute(std::bind(&DLocalFileInfo::setAttribute, this, _1, _2));
    registerHasAttribute(std::bind(&DLocalFileInfo::hasAttribute, this, _1));
    registerExists(std::bind(&DLocalFileInfo::exists, this));
    registerRefresh(std::bind(&DLocalFileInfo::refresh, this));
    registerPermissions(std::bind(&DLocalFileInfo::permissions, this));
    registerSetCustomAttribute(std::bind(&DLocalFileInfo::setCustomAttribute, this, _1, _2, _3, _4));
    registerCustomAttribute(std::bind(&DLocalFileInfo::customAttribute, this, _1, _2));
    registerLastError(std::bind(&DLocalFileInfo::lastError, this));
    // future
    using funcInitQuerierAsync2 = DFileFuture *(DLocalFileInfo::*)(int, QObject *);
    registerInitQuerierAsync2(std::bind((funcInitQuerierAsync2)&DLocalFileInfo::initQuerierAsync, this, _1, _2));
    using funcAttributeAsync2 = DFileFuture *(DLocalFileInfo::*)(AttributeID, int, QObject *)const;
    registerAttributeAsync2(std::bind((funcAttributeAsync2)&DLocalFileInfo::attributeAsync, this, _1, _2, _3));
    using funcAttributeAsync3 = DFileFuture *(DLocalFileInfo::*)(const QByteArray &, const DFileAttributeType, int, QObject *)const;
    registerAttributeAsync3(std::bind((funcAttributeAsync3)&DLocalFileInfo::attributeAsync, this, _1, _2, _3, _4));
    registerExistsAsync(std::bind(&DLocalFileInfo::existsAsync, this, _1, _2));
    registerRefreshAsync(std::bind(&DLocalFileInfo::refreshAsync, this, _1, _2));
    registerPermissionsAsync(std::bind(&DLocalFileInfo::permissionsAsync, this, _1, _2));

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

DFileFuture *DLocalFileInfo::initQuerierAsync(int ioPriority, QObject *parent)
{
    return d->initQuerierAsync(ioPriority, parent);
}

DFileFuture *DLocalFileInfo::attributeAsync(DFileInfo::AttributeID id, int ioPriority, QObject *parent) const
{
    return d->attributeAsync(id, ioPriority, parent);
}

DFileFuture *DLocalFileInfo::attributeAsync(const QByteArray &key, const DFileInfo::DFileAttributeType type, int ioPriority, QObject *parent) const
{
    return d->attributeAsync(key, type, ioPriority, parent);
}

DFileFuture *DLocalFileInfo::existsAsync(int ioPriority, QObject *parent) const
{
    return d->existsAsync(ioPriority, parent);
}

DFileFuture *DLocalFileInfo::refreshAsync(int ioPriority, QObject *parent)
{
    return d->refreshAsync(ioPriority, parent);
}

DFileFuture *DLocalFileInfo::permissionsAsync(int ioPriority, QObject *parent)
{
    return d->permissionsAsync(ioPriority, parent);
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
