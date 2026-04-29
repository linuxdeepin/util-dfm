// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/dfileinfo_p.h"

#include "utils/dmediainfo.h"
#include "utils/dlocalhelper.h"

#include <dfm-io/dfilefuture.h>

#include <QVariant>
#include <QPointer>
#include <QTimer>
#include <QDebug>
#include <QThread>

#include <sys/stat.h>
#include <fcntl.h>
#include <execinfo.h>

USING_IO_NAMESPACE

/************************************************
 * DFileInfoPrivate
 ***********************************************/

typedef struct
{
    DFileInfo::AttributeAsyncCallback callback;
    gpointer user_data;
    DFileInfo::AttributeID id;
    QPointer<DFileInfoPrivate> me;
} QueryFileInfoFromAttributeOp;

static void queryFileInfoFromAttributeCallback(bool ok, void *userData)
{
    QueryFileInfoFromAttributeOp *dataOp = static_cast<QueryFileInfoFromAttributeOp *>(userData);
    if (!dataOp)
        return;

    if (dataOp->callback) {
        if (ok) {
            bool success = false;
            const QVariant &value = dataOp->me->q->attribute(dataOp->id, &success);
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

DFileInfoPrivate::DFileInfoPrivate(DFileInfo *qq)
    : q(qq)
{
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kStandardIsHidden);
    attributesRealizationSelf.push_back(DFileInfo::AttributeID::kOriginalUri);
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

DFileInfoPrivate::DFileInfoPrivate(const DFileInfoPrivate &other)
{
    q = other.q;
}

DFileInfoPrivate &DFileInfoPrivate::operator=(const DFileInfoPrivate &other)
{
    q = other.q;
    return *this;
}

DFileInfoPrivate::~DFileInfoPrivate()
{
    if (gfileinfo) {
        g_object_unref(gfileinfo);
        gfileinfo = nullptr;
    }

    if (gfile) {
        g_object_unref(gfile);
        gfile = nullptr;
    }

    if (gcancellable) {
        g_object_unref(gcancellable);
        gcancellable = nullptr;
    }
}

void DFileInfoPrivate::initNormal()
{
    if (this->gfile) {
        try {
            g_object_unref(this->gfileinfo);
        } catch (...) {
            qWarning() << "DFileInfoPrivate::queryInfoSync - Exception during g_object_unref, continuing...";
        }
        this->gfileinfo = nullptr;
    }

    const QUrl &url = q->uri();
    this->gfile = DLocalHelper::createGFile(url);;
}

void DFileInfoPrivate::attributeExtend(DFileInfo::MediaType type, QList<DFileInfo::AttributeExtendID> ids, DFileInfo::AttributeExtendFuncCallback callback)
{
    if (ids.contains(DFileInfo::AttributeExtendID::kExtendMediaDuration)
        || ids.contains(DFileInfo::AttributeExtendID::kExtendMediaWidth)
        || ids.contains(DFileInfo::AttributeExtendID::kExtendMediaHeight)) {

        const QString &filePath = q->attribute(DFileInfo::AttributeID::kStandardFilePath, nullptr).toString();
        if (!filePath.isEmpty()) {
            mediaType = type;
            extendIDs = ids;
            attributeExtendFuncCallback = callback;

            this->mediaInfo.reset(new DMediaInfo(filePath));
            this->mediaInfo->startReadInfo(std::bind(&DFileInfoPrivate::attributeExtendCallback, this));
        } else {
            if (callback)
                callback(false, {});
        }
    }
}

DFileFuture *DFileInfoPrivate::attributeExtend(DFileInfo::MediaType type, QList<DFileInfo::AttributeExtendID> ids, int ioPriority, QObject *parent)
{
    Q_UNUSED(ioPriority);

    if (ids.contains(DFileInfo::AttributeExtendID::kExtendMediaDuration)
        || ids.contains(DFileInfo::AttributeExtendID::kExtendMediaWidth)
        || ids.contains(DFileInfo::AttributeExtendID::kExtendMediaHeight)) {

        DFileFuture *future = new DFileFuture(parent);

        const QString &filePath = q->attribute(DFileInfo::AttributeID::kStandardFilePath, nullptr).toString();
        if (!filePath.isEmpty()) {
            mediaType = type;
            extendIDs = ids;
            this->future = future;

            this->mediaInfo.reset(new DMediaInfo(filePath));
            this->mediaInfo->startReadInfo(std::bind(&DFileInfoPrivate::attributeExtendCallback, this));

            return future;
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

bool DFileInfoPrivate::cancelAttributeExtend()
{
    if (this->mediaInfo)
        this->mediaInfo->stopReadInfo();
    return true;
}

bool DFileInfoPrivate::cancelAttributes()
{
    if (gcancellable)
        g_cancellable_cancel(gcancellable);

    return cancelAttributeExtend();
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

        if (this->future)
            this->future->infoMedia(uri, map);
    }
}

void DFileInfoPrivate::setErrorFromGError(GError *gerror)
{
    if (!gerror)
        return;

    if (g_error_matches(gerror, G_IO_ERROR,G_IO_ERROR_FAILED) &&
            QString(gerror->message).contains(strerror(EHOSTDOWN))) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_HOST_IS_DOWN);
        error.setMessage(gerror->message);
        return;
    }

    error.setCode(DFMIOErrorCode(gerror->code));
    if (error.code() == DFMIOErrorCode::DFM_IO_ERROR_FAILED)
        error.setMessage(gerror->message);
}

bool DFileInfoPrivate::queryInfoSync()
{
    QMutexLocker lk(&mutex);

    if (!infoReseted && this->gfileinfo) {
        initFinished = true;
        return true;
    }

    initNormal();

    if (!gfile)
        return false;

    g_autoptr(GError) gerror = nullptr;
    GFileInfo *fileinfo = nullptr;
    
    try {
        // 记录调用信息，便于调试崩溃
        qDebug() << "DFileInfoPrivate::queryInfoSync - Attempting g_file_query_info for URI:" << uri.toString()
                 << "with attributes:" << (attributes ? attributes : "null");
        
        checkAndResetCancel();
        
        // 关键调用：添加try/catch保护
        fileinfo = g_file_query_info(gfile, attributes, GFileQueryInfoFlags(flag), gcancellable, &gerror);
        
        qDebug() << "DFileInfoPrivate::queryInfoSync - g_file_query_info completed successfully";
        
    } catch (const std::exception &e) {
        // 捕获C++标准异常
        qCritical() << "DFileInfoPrivate::queryInfoSync - C++ exception caught during g_file_query_info:"
                   << e.what() << "for URI:" << uri.toString();
        return false;
    } catch (...) {
        // 捕获所有其他异常
        qCritical() << "DFileInfoPrivate::queryInfoSync - Unknown exception caught during g_file_query_info for URI:"
                   << uri.toString();
        return false;
    }

    // 处理GIO错误
    if (gerror) {
        qWarning() << "DFileInfoPrivate::queryInfoSync - GError occurred: domain=" << gerror->domain
                   << ", code=" << gerror->code << ", message=" << gerror->message
                   << "for URI:" << uri.toString();
        setErrorFromGError(gerror);
    }

    if (!fileinfo) {
        qCritical() << "DFileInfoPrivate::queryInfoSync - Failed to query file info for URI:" << uri.toString()
                   << ", error:" << error.code() << error.errorMsg();
        return false;
    }

    // 安全地替换旧的gfileinfo
    if (this->gfileinfo) {
        try {
            g_object_unref(this->gfileinfo);
        } catch (...) {
            qWarning() << "DFileInfoPrivate::queryInfoSync - Exception during g_object_unref, continuing...";
        }
        this->gfileinfo = nullptr;
    }
    
    this->gfileinfo = fileinfo;
    initFinished = true;
    
    qDebug() << "DFileInfoPrivate::queryInfoSync - Successfully queried file info for URI:" << uri.toString();
    return true;
}

void DFileInfoPrivate::queryInfoAsync(int ioPriority, DFileInfo::InitQuerierAsyncCallback func, void *userData)
{
    QMutexLocker lk(&mutex);
    initNormal();
    const char *attributes = q->queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = q->queryInfoFlag();

    QueryInfoAsyncOp *dataOp = g_new0(QueryInfoAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;
    dataOp->me = this;
    checkAndResetCancel();
    g_file_query_info_async(this->gfile, attributes, GFileQueryInfoFlags(flag), ioPriority, gcancellable, queryInfoAsyncCallback, dataOp);
}

QVariant DFileInfoPrivate::attributesBySelf(DFileInfo::AttributeID id)
{
    QVariant retValue;
    switch (id) {
    case DFileInfo::AttributeID::kStandardIsHidden: {
        retValue = DLocalHelper::fileIsHidden(q, {});
        break;
    }
    case DFileInfo::AttributeID::kTimeCreated: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint64_t ret = 0;
        {
            QMutexLocker lk(&mutex);
            ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        }
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            if (!url.isLocalFile())
                return QVariant();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret != 0)
                return QVariant();

            return statxBuffer.stx_btime.tv_sec > 0
                    ? quint64(statxBuffer.stx_btime.tv_sec)
                    : quint64(statxBuffer.stx_ctime.tv_sec);
        }
        return qulonglong(ret);
    }
    case DFileInfo::AttributeID::kTimeCreatedUsec: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint32_t ret = 0;
        {
            QMutexLocker lk(&mutex);
            ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        }
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            if (!url.isLocalFile())
                return QVariant();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret != 0)
                return QVariant();

            return statxBuffer.stx_btime.tv_nsec > 0
                    ? statxBuffer.stx_btime.tv_nsec / 1000000
                    : statxBuffer.stx_ctime.tv_nsec / 1000000;
        }
        return QVariant(ret);
    }
    case DFileInfo::AttributeID::kTimeModified: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint64_t ret = 0;
        {
            QMutexLocker lk(&mutex);
            ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        }
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            if (!url.isLocalFile())
                return QVariant();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret != 0)
                return QVariant();

            return statxBuffer.stx_mtime.tv_sec > 0
                    ? quint64(statxBuffer.stx_mtime.tv_sec)
                    : quint64(statxBuffer.stx_ctime.tv_sec);
        }
        return qulonglong(ret);
    }
    case DFileInfo::AttributeID::kTimeModifiedUsec: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint32_t ret = 0;
        {
            QMutexLocker lk(&mutex);
            ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        }
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            if (!url.isLocalFile())
                return QVariant();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret != 0)
                return QVariant();

            return statxBuffer.stx_mtime.tv_nsec > 0
                    ? statxBuffer.stx_mtime.tv_nsec / 1000000
                    : statxBuffer.stx_ctime.tv_nsec / 1000000;
        }
        return QVariant(ret);
    }
    case DFileInfo::AttributeID::kTimeAccess: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint64_t ret = 0;
        {
            QMutexLocker lk(&mutex);
            ret = g_file_info_get_attribute_uint64(gfileinfo, key.c_str());
        }
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            if (!url.isLocalFile())
                return QVariant();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret != 0)
                return QVariant();

            return statxBuffer.stx_atime.tv_sec > 0
                    ? quint64(statxBuffer.stx_atime.tv_sec)
                    : quint64(statxBuffer.stx_ctime.tv_sec);
        }
        return qulonglong(ret);
    }
    case DFileInfo::AttributeID::kTimeAccessUsec: {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return QVariant();
        uint32_t ret = 0;
        {
            QMutexLocker lk(&mutex);
            ret = g_file_info_get_attribute_uint32(gfileinfo, key.c_str());
        }
        if (ret == 0) {
            struct statx statxBuffer;
            unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
            const QUrl &url = q->uri();
            if (!url.isLocalFile())
                return QVariant();
            int ret = statx(AT_FDCWD, url.path().toStdString().data(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &statxBuffer);
            if (ret != 0)
                return QVariant();

            return statxBuffer.stx_atime.tv_nsec > 0
                    ? statxBuffer.stx_atime.tv_nsec / 1000000
                    : statxBuffer.stx_ctime.tv_nsec / 1000000;
        }
        return QVariant(ret);
    }
    case DFileInfo::AttributeID::kOriginalUri: {
        QMutexLocker lk(&mutex);
        if (gfile)
            return QUrl(g_file_get_uri(gfile));
        return uri;
    }
    default:
        return retValue;
    }
    return retValue;
}

QVariant DFileInfoPrivate::attributesFromUrl(DFileInfo::AttributeID id)
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
        g_autofree gchar *name = g_path_get_dirname(q->uri().path().toStdString().c_str());
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
    case DFileInfo::AttributeID::kOriginalUri: {
        QMutexLocker lk(&mutex);
        if (gfile)
            return QUrl(g_file_get_uri(gfile));
        return uri;
    }
    default:
        break;
    }

    return retValue;
}

void DFileInfoPrivate::checkAndResetCancel()
{
    if (gcancellable) {
        if (!g_cancellable_is_cancelled(gcancellable))
            g_cancellable_cancel(gcancellable);
        g_cancellable_reset(gcancellable);
        return;
    }
    gcancellable = g_cancellable_new();
}

DFileFuture *DFileInfoPrivate::initQuerierAsync(int ioPriority, QObject *parent) const
{
    {
        QMutexLocker lk(&mutex);
        const_cast<DFileInfoPrivate *>(this)->initNormal();
    }
    const char *attributes = q->queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = q->queryInfoFlag();

    DFileFuture *future = new DFileFuture(parent);
    QueryInfoAsyncOp2 *dataOp = g_new0(QueryInfoAsyncOp2, 1);
    dataOp->future = future;
    dataOp->me = const_cast<DFileInfoPrivate *>(this);

    const_cast<DFileInfoPrivate *>(this)->checkAndResetCancel();
    g_file_query_info_async(this->gfile, attributes, GFileQueryInfoFlags(flag), ioPriority, gcancellable, queryInfoAsyncCallback2, dataOp);
    return future;
}

QFuture<void> DFileInfoPrivate::refreshAsync()
{
    if (refreshing)
        return futureRefresh;

    refreshing = true;

    if (futureRefresh.isRunning())
        return futureRefresh;

    stoped = false;
    futureRefresh = QtConcurrent::run([=]() {
        if (stoped) {
            refreshing = false;
            return;
        }

        queryInfoSync();

        if (stoped) {
            refreshing = false;
            return;
        }
        cacheAttributes();
        fileExists = exists();
        refreshing = false;
    });
    return futureRefresh;
}

void DFileInfoPrivate::cacheAttributes()
{
    QMap<DFileInfo::AttributeID, QVariant> tmp;
    for (const auto &[id, key] : DLocalHelper::attributeInfoMapFunc()) {
        tmp.insert(id, q->attribute(id));
    }

    tmp.insert(DFileInfo::AttributeID::kAccessPermissions, QVariant::fromValue(permissions()));
    cacheing = true;
    caches = tmp;
    cacheing = false;
}

DFile::Permissions DFileInfoPrivate::permissions() const
{
    DFile::Permissions retValue = DFile::Permission::kNoPermission;

    if (!initFinished) {
        bool succ = const_cast<DFileInfoPrivate *>(this)->queryInfoSync();
        if (!succ)
            return retValue;
    }

    const QVariant &value = q->attribute(DFileInfo::AttributeID::kUnixMode);
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

bool DFileInfoPrivate::exists() const
{
    QMutexLocker lk(&mutex);
    if (!gfileinfo)
        return false;
    return g_file_info_get_file_type(gfileinfo) != G_FILE_TYPE_UNKNOWN;
}

void DFileInfoPrivate::queryInfoAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    QueryInfoAsyncOp *data = static_cast<QueryInfoAsyncOp *>(userData);
    if (!data)
        return;

    GFile *file = G_FILE(sourceObject);
    if (!file) {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback - Source object is not a GFile";

        if (data->callback)
            data->callback(false, data->userData);

        freeQueryInfoAsyncOp(data);
        return;
    }

    g_autoptr(GError) gerror = nullptr;
    GFileInfo *fileinfo = nullptr;
    
    try {
        qDebug() << "DFileInfoPrivate::queryInfoAsyncCallback - Attempting g_file_query_info_finish";
        
        // 关键调用：添加try/catch保护
        fileinfo = g_file_query_info_finish(file, res, &gerror);
        
        qDebug() << "DFileInfoPrivate::queryInfoAsyncCallback - g_file_query_info_finish completed";
        
    } catch (const std::exception &e) {
        qCritical() << "DFileInfoPrivate::queryInfoAsyncCallback - C++ exception caught during g_file_query_info_finish:"
                   << e.what();
        if (data->callback)
            data->callback(false, data->userData);
        freeQueryInfoAsyncOp(data);
        return;
    } catch (...) {
        qCritical() << "DFileInfoPrivate::queryInfoAsyncCallback - Unknown exception caught during g_file_query_info_finish";
        if (data->callback)
            data->callback(false, data->userData);
        freeQueryInfoAsyncOp(data);
        return;
    }

    // 处理GIO错误
    if (gerror) {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback - GError occurred: domain=" << gerror->domain
                   << ", code=" << gerror->code << ", message=" << gerror->message;
        
        if (data->me)
            data->me->setErrorFromGError(gerror);

        if (data->callback)
            data->callback(false, data->userData);

        freeQueryInfoAsyncOp(data);
        return;
    }

    if (!fileinfo) {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback - fileinfo is null (no error)";
    }

    if (data->me) {
        QMutexLocker lk(&data->me->mutex);
        // 安全地更新gfileinfo
        if (data->me->gfileinfo) {
            try {
                g_object_unref(data->me->gfileinfo);
            } catch (...) {
                qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback - Exception during g_object_unref, continuing...";
            }
        }
        data->me->gfileinfo = fileinfo;
        data->me->initFinished = true;
        
        qDebug() << "DFileInfoPrivate::queryInfoAsyncCallback - Successfully updated file info";
    }

    if (data->callback) {
        bool success = fileinfo != nullptr;
        qDebug() << "DFileInfoPrivate::queryInfoAsyncCallback - Calling callback with success:" << success;
        data->callback(success, data->userData);
    }

    freeQueryInfoAsyncOp(data);
}

void DFileInfoPrivate::queryInfoAsyncCallback2(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    QueryInfoAsyncOp2 *data = static_cast<QueryInfoAsyncOp2 *>(userData);
    if (!data) {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback2 - User data is null";
        return;
    }

    DFileFuture *future = data->future;
    if (!future) {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback2 - Future is null";
        freeQueryInfoAsyncOp2(data);
        return;
    }

    GFile *file = G_FILE(sourceObject);
    if (!file) {
        qCritical() << "DFileInfoPrivate::queryInfoAsyncCallback2 - Source object is not a GFile";
        freeQueryInfoAsyncOp2(data);
        return;
    }

    g_autoptr(GError) gerror = nullptr;
    GFileInfo *fileinfo = nullptr;
    
    try {
        qDebug() << "DFileInfoPrivate::queryInfoAsyncCallback2 - Attempting g_file_query_info_finish";
        
        // 关键调用：添加try/catch保护
        fileinfo = g_file_query_info_finish(file, res, &gerror);
        
        qDebug() << "DFileInfoPrivate::queryInfoAsyncCallback2 - g_file_query_info_finish completed";
        
    } catch (const std::exception &e) {
        qCritical() << "DFileInfoPrivate::queryInfoAsyncCallback2 - C++ exception caught during g_file_query_info_finish:"
                   << e.what();
        freeQueryInfoAsyncOp2(data);
        return;
    } catch (...) {
        qCritical() << "DFileInfoPrivate::queryInfoAsyncCallback2 - Unknown exception caught during g_file_query_info_finish";
        freeQueryInfoAsyncOp2(data);
        return;
    }

    // 处理GIO错误
    if (gerror) {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback2 - GError occurred: domain=" << gerror->domain
                   << ", code=" << gerror->code << ", message=" << gerror->message;
        
        if (data->me)
            data->me->setErrorFromGError(gerror);
        
        // 即使有错误，也要标记future为完成
        future->finished();
        
        freeQueryInfoAsyncOp2(data);
        return;
    }

    if (!fileinfo) {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback2 - fileinfo is null (no error)";
    }

    if (data->me) {
        // 安全地更新gfileinfo
        QMutexLocker lk(&data->me->mutex);
        if (data->me->gfileinfo) {
            try {
                g_object_unref(data->me->gfileinfo);
            } catch (...) {
                qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback2 - Exception during g_object_unref, continuing...";
            }
        }
        data->me->gfileinfo = fileinfo;
        data->me->initFinished = true;
        
        qDebug() << "DFileInfoPrivate::queryInfoAsyncCallback2 - Successfully updated file info";
        
        future->finished();
    } else {
        qWarning() << "DFileInfoPrivate::queryInfoAsyncCallback2 - DFileInfoPrivate instance is null";
    }

    freeQueryInfoAsyncOp2(data);
}

void DFileInfoPrivate::freeQueryInfoAsyncOp(DFileInfoPrivate::QueryInfoAsyncOp *op)
{
    op->callback = nullptr;
    op->userData = nullptr;
    op->me = nullptr;
    g_free(op);
}

void DFileInfoPrivate::freeQueryInfoAsyncOp2(DFileInfoPrivate::QueryInfoAsyncOp2 *op)
{
    op->me = nullptr;
    g_free(op);
}

/************************************************
 * DFileInfo
 ***********************************************/

DFileInfo::DFileInfo(const QUrl &uri, const char *attributes, const FileQueryInfoFlags flag)
    : d(new DFileInfoPrivate(this))
{
    d->uri = uri;
    d->attributes = strdup(attributes);
    d->flag = flag;
}

DFileInfo::DFileInfo(const QUrl &uri, void *fileInfo, const char *attributes, const DFileInfo::FileQueryInfoFlags flag)
    : DFileInfo(uri, attributes, flag)
{
    d->initFinished = true;
    d->gfileinfo = static_cast<GFileInfo *>(fileInfo);
}

DFileInfo::DFileInfo(const DFileInfo &info)
    : d(new DFileInfoPrivate(this))
{
    d->uri = info.d->uri;
    d->attributes = strdup(info.d->attributes);
    d->flag = info.d->flag;

    d->extendIDs = info.d->extendIDs;
    d->mediaType = DFileInfo::MediaType::kGeneral;

    d->attributesRealizationSelf = info.d->attributesRealizationSelf;
    d->attributesNoBlockIO = info.d->attributesNoBlockIO;
    d->gfile = g_file_dup(info.d->gfile);
    d->gfileinfo = g_file_info_dup(info.d->gfileinfo);
    d->initFinished = info.d->initFinished.load(std::memory_order_release);
    d->infoReseted = info.d->infoReseted.load(std::memory_order_release);

    d->stoped = info.d->stoped.load(std::memory_order_release);
    d->fileExists = info.d->fileExists.load(std::memory_order_release);
    d->caches = info.d->caches;
    d->cacheing = info.d->cacheing.load(std::memory_order_release);
}

DFileInfo &DFileInfo::operator=(const DFileInfo &info)
{
    d = new DFileInfoPrivate(this);
    d->uri = info.d->uri;
    d->attributes = strdup(info.d->attributes);
    d->flag = info.d->flag;

    d->extendIDs = info.d->extendIDs;
    d->mediaType = DFileInfo::MediaType::kGeneral;

    d->attributesRealizationSelf = info.d->attributesRealizationSelf;
    d->attributesNoBlockIO = info.d->attributesNoBlockIO;
    d->gfile = g_file_dup(info.d->gfile);
    d->gfileinfo = g_file_info_dup(info.d->gfileinfo);
    d->initFinished = info.d->initFinished.load(std::memory_order_release);
    d->infoReseted = info.d->infoReseted.load(std::memory_order_release);

    d->stoped = info.d->stoped.load(std::memory_order_release);
    d->fileExists = info.d->fileExists.load(std::memory_order_release);
    d->caches = info.d->caches;
    d->cacheing = info.d->cacheing.load(std::memory_order_release);
    return *this;
}

DFileInfo::~DFileInfo()
{
    free(d->attributes);
}

bool DFileInfo::initQuerier()
{
    return d->queryInfoSync();
}

QVariant DFileInfo::attribute(DFileInfo::AttributeID id, bool *success) const
{
    if (!d->initFinished) {
        bool succ = const_cast<DFileInfoPrivate *>(d.data())->queryInfoSync();
        if (!succ) {
            if (!d->attributesNoBlockIO.contains(id)) {
                if (id == DFileInfo::AttributeID::kStandardIsHidden) {
                    const auto &fileName = d->uri.fileName();
                    return fileName.startsWith('.');
                }

                return QVariant();
            } else {
                return const_cast<DFileInfoPrivate *>(d.data())->attributesFromUrl(id);
            }
        }
    }

    QVariant retValue;
    if (id > DFileInfo::AttributeID::kCustomStart) {
        const QString &path = d->uri.path();
        QMutexLocker lk(&d->mutex);
        retValue = DLocalHelper::customAttributeFromPathAndInfo(path, d->gfileinfo, id);
    } else {
        if (d->gfileinfo) {
            DFMIOErrorCode errorCode(DFM_IO_ERROR_NONE);
            if (!d->attributesRealizationSelf.contains(id)) {
                QMutexLocker lk(&d->mutex);
                retValue = DLocalHelper::attributeFromGFileInfo(d->gfileinfo, id, errorCode);
                if (errorCode != DFM_IO_ERROR_NONE)
                    const_cast<DFileInfoPrivate *>(d.data())->error.setCode(errorCode);
            } else {
                retValue = const_cast<DFileInfoPrivate *>(d.data())->attributesBySelf(id);
            }
        }
    }
    if (success)
        *success = retValue.isValid();

    if (!retValue.isValid())
        retValue = std::get<1>(DLocalHelper::attributeInfoMapFunc().at(id));
    return retValue;
}

void DFileInfo::initQuerierAsync(int ioPriority, DFileInfo::InitQuerierAsyncCallback func, void *userData)
{
    {
        QMutexLocker lk(&d->mutex);
        d->initNormal();
    }
    const char *attributes = queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = queryInfoFlag();

    DFileInfoPrivate::QueryInfoAsyncOp *dataOp = g_new0(DFileInfoPrivate::QueryInfoAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;
    dataOp->me = d.data();

    g_file_query_info_async(d->gfile, attributes, GFileQueryInfoFlags(flag), ioPriority, nullptr, DFileInfoPrivate::queryInfoAsyncCallback, dataOp);
}

void DFileInfo::attributeAsync(DFileInfo::AttributeID id, bool *success, int ioPriority, DFileInfo::AttributeAsyncCallback func, void *userData)
{
    if (!d->initFinished) {
        // query async
        QueryFileInfoFromAttributeOp *dataOp = g_new0(QueryFileInfoFromAttributeOp, 1);
        dataOp->callback = func;
        dataOp->user_data = userData;
        dataOp->id = id;
        dataOp->me = d.data();

        d->queryInfoAsync(ioPriority, queryFileInfoFromAttributeCallback, dataOp);
        return;
    }

    const QVariant &value = attribute(id, success);
    if (func)
        func(success, userData, value);
}

DFileFuture *DFileInfo::initQuerierAsync(int ioPriority, QObject *parent)
{
    {
        QMutexLocker lk(&d->mutex);
        d->initNormal();
    }
    const char *attributes = queryAttributes();
    const DFileInfo::FileQueryInfoFlags flag = queryInfoFlag();

    DFileFuture *future = new DFileFuture(parent);
    DFileInfoPrivate::QueryInfoAsyncOp2 *dataOp = g_new0(DFileInfoPrivate::QueryInfoAsyncOp2, 1);
    dataOp->future = future;
    dataOp->me = d.data();

    d->checkAndResetCancel();
    g_file_query_info_async(d->gfile, attributes, GFileQueryInfoFlags(flag), ioPriority, d->gcancellable, DFileInfoPrivate::queryInfoAsyncCallback2, dataOp);
    return future;
}

DFileFuture *DFileInfo::attributeAsync(DFileInfo::AttributeID id, int ioPriority, QObject *parent) const
{
    DFileFuture *futureRet = new DFileFuture(parent);
    if (!d->initFinished) {
        DFileFuture *future = d->initQuerierAsync(ioPriority, nullptr);
        QObject::connect(future, &DFileFuture::finished, d.data(), [=]() {
            if (!future->hasError()) {
                futureRet->infoAttribute(id, attribute(id));
                futureRet->finished();
            }
            future->deleteLater();
        });
    }
    QTimer::singleShot(0, [=]() {
        futureRet->infoAttribute(id, attribute(id));
        futureRet->finished();
    });
    return futureRet;
}

DFileFuture *DFileInfo::attributeAsync(const QByteArray &key, const DFileInfo::DFileAttributeType type, int ioPriority, QObject *parent) const
{
    DFileFuture *futureRet = new DFileFuture(parent);
    if (!d->initFinished) {
        DFileFuture *future = d->initQuerierAsync(ioPriority, nullptr);
        QObject::connect(future, &DFileFuture::finished, d.data(), [=]() {
            if (!future->hasError()) {
                futureRet->infoAttribute(key, customAttribute(key, type));
                futureRet->finished();
            }
            future->deleteLater();
        });
    }

    QTimer::singleShot(0, [=]() {
        futureRet->infoAttribute(key, customAttribute(key, type));
        futureRet->finished();
    });

    return futureRet;
}

DFileFuture *DFileInfo::existsAsync(int ioPriority, QObject *parent) const
{
    DFileFuture *futureRet = new DFileFuture(parent);
    if (!d->initFinished) {
        DFileFuture *future = d->initQuerierAsync(ioPriority, nullptr);
        QObject::connect(future, &DFileFuture::finished, d.data(), [=]() {
            if (!future->hasError()) {
                const bool exists = this->exists();
                futureRet->infoExists(exists);
                futureRet->finished();
            }
            future->deleteLater();
        });
    }
    QTimer::singleShot(0, [=]() {
        const bool exists = this->exists();
        futureRet->infoExists(exists);
        futureRet->finished();
    });
    return futureRet;
}

DFileFuture *DFileInfo::refreshAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = this->initQuerierAsync(ioPriority, parent);
    QObject::connect(future, &DFileFuture::finished, d.data(), [=]() {
        future->finished();
    });
    return future;
}

DFileFuture *DFileInfo::permissionsAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = this->initQuerierAsync(ioPriority, parent);
    QObject::connect(future, &DFileFuture::finished, d.data(), [=]() {
        future->infoPermissions(this->permissions());
        future->finished();
    });
    return future;
}

QFuture<void> DFileInfo::refreshAsync()
{
    d->initFinished = false;
    return d->refreshAsync();
}

bool DFileInfo::hasAttribute(DFileInfo::AttributeID id) const
{
    if (!d->initFinished) {
        bool succ = const_cast<DFileInfoPrivate *>(d.data())->queryInfoSync();
        if (!succ)
            return false;
    }

    QMutexLocker lk(&d->mutex);
    if (d->gfileinfo) {
        const std::string &key = DLocalHelper::attributeStringById(id);
        if (key.empty())
            return false;
        return g_file_info_has_attribute(d->gfileinfo, key.c_str());
    }

    return false;
}

bool DFileInfo::exists() const
{
    if (!d->cacheing && !d->caches.isEmpty())
        return d->fileExists;

    return d->exists();
}

bool DFileInfo::refresh()
{
    d->initFinished = false;
    d->infoReseted = true;
    bool ret = d->queryInfoSync();
    d->infoReseted = false;
    return ret;
}

DFile::Permissions DFileInfo::permissions() const
{
    if (!d->cacheing && !d->caches.isEmpty())
        return d->caches.value(AttributeID::kAccessPermissions).value<DFile::Permissions>();

    return d->permissions();
}

bool DFileInfo::setCustomAttribute(const char *key, const DFileInfo::DFileAttributeType type, const void *value, const DFileInfo::FileQueryInfoFlags flag)
{
    QMutexLocker lk(&d->mutex);
    if (d->gfile) {
        g_autoptr(GError) gerror = nullptr;
        bool ret = g_file_set_attribute(d->gfile, key, GFileAttributeType(type), (gpointer)(value), GFileQueryInfoFlags(flag), nullptr, &gerror);

        if (gerror)
            d->setErrorFromGError(gerror);
        return ret;
    }
    return false;
}

QVariant DFileInfo::customAttribute(const char *key, const DFileInfo::DFileAttributeType type) const
{
    if (!d->initFinished) {
        bool succ = const_cast<DFileInfoPrivate *>(d.data())->queryInfoSync();
        if (!succ)
            return QVariant();
    }

    if (!d->gfileinfo)
        return QVariant();

    switch (type) {
    case DFileInfo::DFileAttributeType::kTypeString: {
        const char *ret = g_file_info_get_attribute_string(d->gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeByteString: {
        const char *ret = g_file_info_get_attribute_byte_string(d->gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeBool: {
        bool ret = g_file_info_get_attribute_boolean(d->gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeUInt32: {
        uint32_t ret = g_file_info_get_attribute_uint32(d->gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeInt32: {
        int32_t ret = g_file_info_get_attribute_int32(d->gfileinfo, key);
        return QVariant(ret);
    }
    case DFileInfo::DFileAttributeType::kTypeUInt64: {
        uint64_t ret = g_file_info_get_attribute_uint64(d->gfileinfo, key);
        return QVariant(qulonglong(ret));
    }
    case DFileInfo::DFileAttributeType::kTypeInt64: {
        int64_t ret = g_file_info_get_attribute_int64(d->gfileinfo, key);
        return QVariant(qulonglong(ret));
    }
    case DFileInfo::DFileAttributeType::kTypeStringV: {
        char **ret = g_file_info_get_attribute_stringv(d->gfileinfo, key);
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

DFMIOError DFileInfo::lastError() const
{
    return d->error;
}

void DFileInfo::attributeExtend(DFileInfo::MediaType type, QList<AttributeExtendID> ids, DFileInfo::AttributeExtendFuncCallback callback)
{
    d->attributeExtend(type, ids, callback);
}

DFileFuture *DFileInfo::attributeExtend(DFileInfo::MediaType type, QList<DFileInfo::AttributeExtendID> ids, int ioPriority, QObject *parent)
{
    return d->attributeExtend(type, ids, ioPriority, parent);
}

bool DFileInfo::cancelAttributeExtend()
{
    return d->cancelAttributeExtend();
}

bool DFileInfo::cancelAttributes()
{
    d->stoped = true;
    if (d->gcancellable)
        g_cancellable_cancel(d->gcancellable);
    cancelAttributeExtend();
    return true;
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
    for (const auto &[id, key] : DLocalHelper::attributeInfoMapFunc()) {
        const QVariant &&value = attribute(id);
        if (value.isValid()) {
            ret.append(std::get<0>(DLocalHelper::attributeInfoMapFunc().at(id)).c_str());
            ret.append(":");
            ret.append(value.toString());
            ret.append("\n");
        }
    }
    return ret;
}

bool DFileInfo::queryAttributeFinished() const
{
    return d->initFinished;
}
