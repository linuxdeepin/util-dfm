// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/dfile_p.h"
#include "utils/dlocalhelper.h"

#include <dfm-io/dfilefuture.h>

#include <QtConcurrent>
#include <QPointer>
#include <QDebug>

#include <gio/gio.h>

#include <sys/stat.h>

USING_IO_NAMESPACE

/************************************************
 * DFilePrivate
 ***********************************************/

DFilePrivate::DFilePrivate(DFile *q)
    : q(q)
{
}

DFilePrivate::~DFilePrivate()
{
    // 确保 cancellable 资源被正确清理
    if (cancellable) {
        g_object_unref(cancellable);
        cancellable = nullptr;
    }
    
    // 清理其他 GObject 资源
    if (iStream) {
        g_object_unref(iStream);
        iStream = nullptr;
    }
    if (oStream) {
        g_object_unref(oStream);
        oStream = nullptr;
    }
    if (ioStream) {
        g_object_unref(ioStream);
        ioStream = nullptr;
    }
}

void DFilePrivate::setError(DFMIOError error)
{
    this->error = error;
}

void DFilePrivate::setErrorFromGError(GError *gerror)
{
    if (!gerror)
        return;

    error.setCode(DFMIOErrorCode(gerror->code));
    if (error.code() == DFMIOErrorCode::DFM_IO_ERROR_FAILED)
        error.setMessage(gerror->message);
}

void DFilePrivate::checkAndResetCancel()
{
    if (cancellable) {
        if (!g_cancellable_is_cancelled(cancellable))
            g_cancellable_cancel(cancellable);
        g_cancellable_reset(cancellable);
        return;
    }
    cancellable = g_cancellable_new();
}

GInputStream *DFilePrivate::inputStream()
{
    if (iStream)
        return iStream;

    if (ioStream) {
        GInputStream *inputStream = g_io_stream_get_input_stream(ioStream);
        if (inputStream)
            return inputStream;
    }

    return nullptr;
}

GOutputStream *DFilePrivate::outputStream()
{
    if (oStream)
        return oStream;

    if (ioStream) {
        GOutputStream *outputStream = g_io_stream_get_output_stream(ioStream);
        if (outputStream)
            return outputStream;
    }

    return nullptr;
}

DFile::Permissions DFilePrivate::permissionsFromGFileInfo(GFileInfo *gfileinfo)
{
    DFile::Permissions retValue = DFile::Permission::kNoPermission;
    if (!gfileinfo)
        return retValue;

    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);
    const quint32 &stMode = g_file_info_get_attribute_uint32(gfileinfo, attributeKey.c_str());
    if (!stMode)
        return retValue;

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

bool DFilePrivate::checkOpenFlags(DFile::OpenFlags *modeIn)
{
    DFile::OpenFlags &mode = *modeIn;

    if (mode & DFile::OpenFlag::kNewOnly) {
        if (q->exists()) {
            error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FLAG_ERROR);
            return false;
        }
    }
    if (mode & DFile::OpenFlag::kExistingOnly) {
        if (!q->exists()) {
            error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FLAG_ERROR);
            return false;
        }
    }
    if ((mode & DFile::OpenFlag::kNewOnly) && (mode & DFile::OpenFlag::kExistingOnly)) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FLAG_ERROR);
        return false;
    }

    // WriteOnly implies Truncate when ReadOnly, Append, and NewOnly are not set.
    if ((mode & DFile::OpenFlag::kWriteOnly) && !(mode & (DFile::OpenFlag::kReadOnly | DFile::OpenFlag::kAppend | DFile::OpenFlag::kNewOnly)))
        mode |= DFile::OpenFlag::kTruncate;

    if (mode & (DFile::OpenFlag::kAppend | DFile::OpenFlag::kNewOnly))
        mode |= DFile::OpenFlag::kWriteOnly;

    if ((mode & (DFile::OpenFlag::kReadOnly | DFile::OpenFlag::kWriteOnly)) == 0) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FLAG_ERROR);
        return false;
    }
    if ((mode & DFile::OpenFlag::kExistingOnly) && !(mode & (DFile::OpenFlag::kReadOnly | DFile::OpenFlag::kWriteOnly))) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FLAG_ERROR);
        return false;
    }

    return true;
}

quint32 DFilePrivate::buildPermissions(DFile::Permissions permission)
{
    quint32 stMode = 0000;
    if (permission.testFlag(DFile::Permission::kExeOwner) | permission.testFlag(DFile::Permission::kExeUser))
        stMode |= S_IXUSR;
    if (permission.testFlag(DFile::Permission::kWriteOwner) | permission.testFlag(DFile::Permission::kWriteUser))
        stMode |= S_IWUSR;
    if (permission.testFlag(DFile::Permission::kReadOwner) | permission.testFlag(DFile::Permission::kReadUser))
        stMode |= S_IRUSR;

    if (permission.testFlag(DFile::Permission::kExeGroup))
        stMode |= S_IXGRP;
    if (permission.testFlag(DFile::Permission::kWriteGroup))
        stMode |= S_IWGRP;
    if (permission.testFlag(DFile::Permission::kReadGroup))
        stMode |= S_IRGRP;

    if (permission.testFlag(DFile::Permission::kExeOther))
        stMode |= S_IXOTH;
    if (permission.testFlag(DFile::Permission::kWriteOther))
        stMode |= S_IWOTH;
    if (permission.testFlag(DFile::Permission::kReadOther))
        stMode |= S_IROTH;
    return stMode;
}

bool DFilePrivate::doOpen(DFile::OpenFlags mode)
{
    if (q->isOpen()) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return false;
    }

    // check mode
    if (!checkOpenFlags(&mode)) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return false;
    }

    const QUrl &&uri = q->uri();
    g_autoptr(GFile) gfile = DLocalHelper::createGFile(uri);
    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();

    if (mode & DFile::OpenFlag::kReadOnly && !(mode & DFile::OpenFlag::kWriteOnly)) {
        if (!q->exists()) {
            return false;
        }
        iStream = (GInputStream *)g_file_read(gfile, cancellable, &gerror);
        if (gerror)
            setErrorFromGError(gerror);

        if (!iStream) {
            return false;
        }
        return true;
    } else if (mode & DFile::OpenFlag::kWriteOnly && !(mode & DFile::OpenFlag::kReadOnly)) {
        if (mode & DFile::OpenFlag::kNewOnly) {
            oStream = (GOutputStream *)g_file_create(gfile, G_FILE_CREATE_NONE, cancellable, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!oStream) {
                return false;
            }
        } else if (mode & DFile::OpenFlag::kAppend) {
            oStream = (GOutputStream *)g_file_append_to(gfile, G_FILE_CREATE_NONE, cancellable, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!oStream) {
                return false;
            }
        } else {
            oStream = (GOutputStream *)g_file_replace(gfile, nullptr, false, G_FILE_CREATE_NONE, cancellable, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!oStream) {
                return false;
            }
        }

        return true;
    } else if (mode & DFile::OpenFlag::kReadOnly && mode & DFile::OpenFlag::kWriteOnly) {
        if (mode & DFile::OpenFlag::kNewOnly) {
            ioStream = (GIOStream *)g_file_create_readwrite(gfile, G_FILE_CREATE_NONE, cancellable, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!ioStream) {
                return false;
            }
        } else if (mode & DFile::OpenFlag::kExistingOnly) {
            ioStream = (GIOStream *)g_file_open_readwrite(gfile, cancellable, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!ioStream) {
                return false;
            }
        } else {
            ioStream = (GIOStream *)g_file_replace_readwrite(gfile, nullptr, false, G_FILE_CREATE_NONE, cancellable, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!ioStream) {
                return false;
            }
        }
        return true;
    } else {
        ioStream = (GIOStream *)g_file_replace_readwrite(gfile, nullptr, false, G_FILE_CREATE_NONE, cancellable, &gerror);
        if (gerror)
            setErrorFromGError(gerror);

        if (!ioStream) {
            return false;
        }
        return true;
    }
}

bool DFilePrivate::doClose()
{
    if (iStream) {
        if (!g_input_stream_is_closed(iStream))
            g_input_stream_close(iStream, nullptr, nullptr);
        g_object_unref(iStream);
        iStream = nullptr;
    }
    if (oStream) {
        if (!g_output_stream_is_closed(oStream))
            g_output_stream_close(oStream, nullptr, nullptr);
        g_object_unref(oStream);
        oStream = nullptr;
    }
    if (ioStream) {
        if (!g_io_stream_is_closed(ioStream))
            g_io_stream_close(ioStream, nullptr, nullptr);
        g_object_unref(ioStream);
        ioStream = nullptr;
    }
    if (cancellable) {
        g_object_unref(cancellable);
        cancellable = nullptr;
    }

    return true;
}

QByteArray DFilePrivate::doReadAll()
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return QByteArray();
    }

    QByteArray dataRet;

    const gsize size = 8192;

    GError *gerror = nullptr;

    while (true) {
        gsize bytesRead;
        char data[size];
        memset(data, 0, size);

        checkAndResetCancel();
        gboolean read = g_input_stream_read_all(inputStream,
                                                data,
                                                size,
                                                &bytesRead,
                                                cancellable,
                                                &gerror);
        if (!read || gerror) {
            if (gerror) {
                setErrorFromGError(gerror);
                g_error_free(gerror);
            }
            break;
        }
        if (bytesRead == 0)
            break;

        dataRet.append(data, bytesRead);
    }

    return dataRet;
}

qint64 DFilePrivate::doWrite(const char *data, qint64 maxSize)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return -1;
    }

    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    gssize write = g_output_stream_write(outputStream,
                                         data,
                                         static_cast<gsize>(maxSize),
                                         cancellable,
                                         &gerror);
    if (gerror) {
        setErrorFromGError(gerror);
        return -1;
    }

    return write;
}

qint64 DFilePrivate::doWrite(const char *data)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return -1;
    }

    gsize bytes_write;
    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    gssize write = g_output_stream_write_all(outputStream,
                                             data,
                                             strlen(data),
                                             &bytes_write,
                                             cancellable,
                                             &gerror);
    if (gerror) {
        setErrorFromGError(gerror);
        return -1;
    }

    return write;
}

qint64 DFilePrivate::doWrite(const QByteArray &data)
{
    return doWrite(data.data(), data.length());
}

void DFilePrivate::readAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    ReadAsyncOp *data = static_cast<ReadAsyncOp *>(userData);
    GInputStream *stream = (GInputStream *)(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    gssize size = g_input_stream_read_finish(stream, res, &gerror);
    if (data->callback)
        data->callback(size, data->userData);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

void DFilePrivate::readQAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    ReadQAsyncOp *data = static_cast<ReadQAsyncOp *>(userData);
    GInputStream *stream = (GInputStream *)(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    gssize size = g_input_stream_read_finish(stream, res, &gerror);
    QByteArray dataRet = size >= 0 ? QByteArray(data->data) : QByteArray();
    if (data->callback)
        data->callback(dataRet, data->userData);

    data->callback = nullptr;
    data->userData = nullptr;
    data->data = nullptr;
    g_free(data);
}

void DFilePrivate::readAllAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    ReadAllAsyncOp *data = static_cast<ReadAllAsyncOp *>(userData);
    GInputStream *stream = (GInputStream *)(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    gsize size = 0;
    bool succ = g_input_stream_read_all_finish(stream, res, &size, &gerror);
    if (!succ || gerror) {
        if (data->callback)
            data->callback(QByteArray(), data->userData);
    }
    if (size == 0) {
        if (data->callback) {
            if (data->me)
                data->callback(data->me->readAllAsyncRet, data->userData);
        }
    }

    if (data->me) {
        data->me->readAllAsyncRet.append(data->data);
        data->me->q->readAllAsync(data->ioPriority, data->callback, data->userData);
    }

    data->callback = nullptr;
    data->userData = nullptr;
    data->data = nullptr;
    data->ioPriority = 0;
    data->me = nullptr;
    g_free(data);
}

void DFilePrivate::writeAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    WriteAsyncOp *data = static_cast<WriteAsyncOp *>(userData);
    GOutputStream *stream = (GOutputStream *)(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    gssize size = g_output_stream_write_finish(stream, res, &gerror);
    if (data->callback)
        data->callback(size, data->userData);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

void DFilePrivate::permissionsAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    if (!data)
        return;

    QPointer<DFilePrivate> me = data->me;
    if (!me)
        return;
    DFileFuture *future = data->future;
    g_autoptr(GFile) gfile = G_FILE(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GFileInfo) gfileinfo = g_file_query_info_finish(gfile, res, &gerror);
    if (gerror) {
        me->setErrorFromGError(gerror);
        me = nullptr;
        future = nullptr;
        g_free(data);
        return;
    }
    auto permissions = data->me->permissionsFromGFileInfo(gfileinfo);
    future->infoPermissions(permissions);
    future->finished();

    me = nullptr;
    future = nullptr;
    g_free(data);
}

void DFilePrivate::existsAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    if (!data)
        return;

    QPointer<DFilePrivate> me = data->me;
    if (!me)
        return;
    DFileFuture *future = data->future;
    g_autoptr(GFile) gfile = G_FILE(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GFileInfo) gfileinfo = g_file_query_info_finish(gfile, res, &gerror);
    if (gerror) {
        me->setErrorFromGError(gerror);
        me = nullptr;
        future = nullptr;
        g_free(data);
        return;
    }

    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kStandardType);
    const quint32 exists = g_file_info_get_attribute_uint32(gfileinfo, attributeKey.c_str());

    future->infoExists(exists != G_FILE_TYPE_UNKNOWN);
    future->finished();

    me = nullptr;
    future = nullptr;
    g_free(data);
}

void DFilePrivate::sizeAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    if (!data)
        return;

    QPointer<DFilePrivate> me = data->me;
    if (!me)
        return;
    DFileFuture *future = data->future;
    g_autoptr(GFile) gfile = G_FILE(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GFileInfo) gfileinfo = g_file_query_info_finish(gfile, res, &gerror);
    if (gerror) {
        me->setErrorFromGError(gerror);
        me = nullptr;
        future = nullptr;
        g_free(data);
        return;
    }

    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kStandardSize);
    const quint64 size = g_file_info_get_attribute_uint64(gfileinfo, attributeKey.c_str());

    future->infoSize(size);
    future->finished();

    me = nullptr;
    future = nullptr;
    g_free(data);
}

void DFilePrivate::flushAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    QPointer<DFilePrivate> me = data->me;
    DFileFuture *future = data->future;
    g_autoptr(GOutputStream) stream = G_OUTPUT_STREAM(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    g_output_stream_flush_finish(stream, res, &gerror);
    if (gerror) {
        me->setErrorFromGError(gerror);
        me = nullptr;
        future = nullptr;
        g_free(data);
        return;
    }
    future->finished();

    me = nullptr;
    future = nullptr;
    g_free(data);
}

void DFilePrivate::writeAsyncFutureCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    QPointer<DFilePrivate> me = data->me;
    DFileFuture *future = data->future;
    GOutputStream *stream = (GOutputStream *)(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    gssize size = g_output_stream_write_finish(stream, res, &gerror);
    if (gerror) {
        me->setErrorFromGError(gerror);
        me = nullptr;
        future = nullptr;
        g_free(data);
        return;
    }
    future->writeAsyncSize(size);
    future->finished();

    me = nullptr;
    future = nullptr;
    g_free(data);
}

void DFilePrivate::readAsyncFutureCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    ReadAllAsyncFutureOp *data = static_cast<ReadAllAsyncFutureOp *>(userData);
    GInputStream *stream = (GInputStream *)(sourceObject);
    QPointer<DFilePrivate> me = data->me;
    DFileFuture *future = data->future;

    g_autoptr(GError) gerror = nullptr;
    gsize size = 0;
    bool succ = g_input_stream_read_all_finish(stream, res, &size, &gerror);
    if (!succ || gerror) {
        future->setError(DFMIOErrorCode(gerror->code));
        me->setErrorFromGError(gerror);
    }

    future->readData(data->data);
    future->finished();

    data->future = nullptr;
    data->me = nullptr;
    g_free(data);
}

/************************************************
 * DFile
 ***********************************************/

DFile::DFile(const QUrl &uri)
    : d(new DFilePrivate(this))
{
    d->uri = uri;
}

DFile::DFile(const QString &path)
    : d(new DFilePrivate(this))
{
    d->uri = QUrl::fromLocalFile(path);
}

DFile::~DFile()
{
    close();
}

QUrl DFile::uri() const
{
    return d->uri;
}

bool DFile::isOpen() const
{
    return d->isOpen;
}

qint64 DFile::size() const
{
    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);

    g_autoptr(GError) gerror = nullptr;
    d->checkAndResetCancel();
    g_autoptr(GFileInfo) fileInfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, d->cancellable, &gerror);

    if (gerror)
        d->setErrorFromGError(gerror);

    if (fileInfo) {
        goffset size = g_file_info_get_size(fileInfo);
        return qint64(size);
    }

    return -1;
}

bool DFile::exists() const
{
    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);
    d->checkAndResetCancel();
    return g_file_query_file_type(gfile, G_FILE_QUERY_INFO_NONE, d->cancellable) != G_FILE_TYPE_UNKNOWN;
}

qint64 DFile::pos() const
{
    GInputStream *inputStream = d->inputStream();
    if (inputStream) {
        // seems g_seekable_can_seek only support local file, survey after. todo lanxs
        gboolean canSeek = G_IS_SEEKABLE(inputStream) /*&& g_seekable_can_seek(G_SEEKABLE(inputStream))*/;
        if (!canSeek) {
            return -1;
        }

        GSeekable *seekable = G_SEEKABLE(inputStream);
        if (!seekable) {
            return -2;
        }

        goffset pos = g_seekable_tell(seekable);

        return qint64(pos);
    }

    GOutputStream *outputStream = d->outputStream();
    if (outputStream){
        // seems g_seekable_can_seek only support local file, survey after. todo lanxs
        gboolean canSeek = G_IS_SEEKABLE(outputStream) /*&& g_seekable_can_seek(G_SEEKABLE(inputStream))*/;
        if (!canSeek) {
            return -3;
        }

        GSeekable *seekable = G_SEEKABLE(outputStream);
        if (!seekable) {
            return -4;
        }

        goffset pos = g_seekable_tell(seekable);

        return qint64(pos);
    }

    d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
    return -5;

}

DFile::Permissions DFile::permissions() const
{
    DFile::Permissions retValue = DFile::Permission::kNoPermission;

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);

    g_autoptr(GError) gerror = nullptr;
    d->checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);
    if (attributeKey.empty())
        return retValue;
    g_autoptr(GFileInfo) fileInfo = g_file_query_info(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, d->cancellable, &gerror);

    if (gerror)
        d->setErrorFromGError(gerror);

    if (!fileInfo)
        return retValue;

    return d->permissionsFromGFileInfo(fileInfo);
}

DFMIOError DFile::lastError() const
{
    return d->error;
}

bool DFile::open(DFile::OpenFlags mode)
{
    d->isOpen = d->doOpen(mode);

    return d->isOpen;
}

bool DFile::close()
{
    if (d->isOpen) {
        if (d->doClose())
            d->isOpen = false;
        else
            return false;
    }

    return true;
}

bool DFile::cancel()
{
    if (d->cancellable && !g_cancellable_is_cancelled(d->cancellable))
        g_cancellable_cancel(d->cancellable);
    return true;
}

bool DFile::seek(qint64 pos, DFile::SeekType type) const
{
    GInputStream *inputStream = d->inputStream();
    if (inputStream) {
        // seems g_seekable_can_seek only support local file, survey after. todo lanxs
        gboolean canSeek = G_IS_SEEKABLE(inputStream) /*&& g_seekable_can_seek(G_SEEKABLE(inputStream))*/;
        if (!canSeek) {
            return false;
        }

        GSeekable *seekable = G_SEEKABLE(inputStream);
        if (!seekable) {
            return false;
        }

        bool ret = false;
        GError *gerror = nullptr;
        GSeekType gtype = G_SEEK_CUR;
        switch (type) {
        case DFile::SeekType::kBegin:
            gtype = G_SEEK_SET;
            break;
        case DFile::SeekType::kEnd:
            gtype = G_SEEK_END;
            break;

        default:
            break;
        }

        d->checkAndResetCancel();
        ret = g_seekable_seek(seekable, pos, gtype, d->cancellable, &gerror);
        if (gerror) {
            qCritical() << " seek err code = " << gerror->code
                        << " , seek err msg = " << gerror->message;
            d->setErrorFromGError(gerror);
            g_error_free(gerror);
        }

        return ret;
    }

    GOutputStream *out = d->outputStream();
    if (out) {
        // seems g_seekable_can_seek only support local file, survey after. todo lanxs
        gboolean canSeek = G_IS_SEEKABLE(out) /*&& g_seekable_can_seek(G_SEEKABLE(inputStream))*/;
        if (!canSeek) {
            return false;
        }

        GSeekable *seekable = G_SEEKABLE(out);
        if (!seekable) {
            return false;
        }

        bool ret = false;
        GError *gerror = nullptr;
        GSeekType gtype = G_SEEK_CUR;
        switch (type) {
        case DFile::SeekType::kBegin:
            gtype = G_SEEK_SET;
            break;
        case DFile::SeekType::kEnd:
            gtype = G_SEEK_END;
            break;

        default:
            break;
        }

        d->checkAndResetCancel();
        ret = g_seekable_seek(seekable, pos, gtype, d->cancellable, &gerror);
        if (gerror) {
            qCritical() << " seek err code = " << gerror->code
                        << " , seek err msg = " << gerror->message;
            d->setErrorFromGError(gerror);
            g_error_free(gerror);
        }

        return ret;
    }

    d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
    return false;


}

bool DFile::flush()
{
    GOutputStream *outputStream = d->outputStream();
    if (!outputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return false;
    }

    g_autoptr(GError) gerror = nullptr;
    d->checkAndResetCancel();
    gboolean ret = g_output_stream_flush(outputStream, d->cancellable, &gerror);

    if (gerror)
        d->setErrorFromGError(gerror);

    return ret;
}

bool DFile::setPermissions(Permissions permission)
{
    quint32 stMode = d->buildPermissions(permission);

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);
    g_autoptr(GError) gerror = nullptr;
    d->checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);
    bool succ = g_file_set_attribute_uint32(gfile, attributeKey.c_str(), stMode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, d->cancellable, &gerror);
    if (gerror)
        d->setErrorFromGError(gerror);
    return succ;
}

qint64 DFile::read(char *data, qint64 maxSize)
{
    GInputStream *inputStream = d->inputStream();
    if (!inputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return -1;
    }

    g_autoptr(GError) gerror = nullptr;
    d->checkAndResetCancel();
    gssize read = g_input_stream_read(inputStream,
                                      data,
                                      static_cast<gsize>(maxSize),
                                      d->cancellable,
                                      &gerror);
    if (gerror) {
        d->setErrorFromGError(gerror);
        return -1;
    }

    return read;
}

QByteArray DFile::read(qint64 maxSize)
{
    GInputStream *inputStream = d->inputStream();
    if (!inputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return QByteArray();
    }

    char data[maxSize + 1];
    memset(&data, 0, maxSize + 1);

    g_autoptr(GError) gerror = nullptr;
    d->checkAndResetCancel();
    g_input_stream_read(inputStream,
                        data,
                        static_cast<gsize>(maxSize),
                        d->cancellable,
                        &gerror);
    if (gerror) {
        d->setErrorFromGError(gerror);
        return QByteArray();
    }

    return QByteArray(data);
}

QByteArray DFile::readAll()
{
    bool innerOpen { false };
    if (!d->isOpen) {
        if (!open(DFMIO::DFile::OpenFlag::kReadOnly))
            return QByteArray();
        innerOpen = true;
    }
    const auto &bytes { d->doReadAll() };
    if (innerOpen)
        close();
    return bytes;
}

qint64 DFile::write(const char *data, qint64 len)
{
    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return -1;
    }

    return d->doWrite(data, len);
}

qint64 DFile::write(const char *data)
{
    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return -1;
    }

    return d->doWrite(data);
}

qint64 DFile::write(const QByteArray &byteArray)
{
    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return -1;
    }

    return d->doWrite(byteArray);
}

void DFile::readAsync(char *data, qint64 maxSize, int ioPriority, DFile::ReadCallbackFunc func, void *userData)
{
    GInputStream *inputStream = d->inputStream();
    if (!inputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(-1, userData);
        return;
    }

    DFilePrivate::ReadAsyncOp *dataOp = g_new0(DFilePrivate::ReadAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;

    d->checkAndResetCancel();
    g_input_stream_read_async(inputStream,
                              data,
                              static_cast<gsize>(maxSize),
                              ioPriority,
                              d->cancellable,
                              DFilePrivate::readAsyncCallback,
                              dataOp);
}

void DFile::readQAsync(qint64 maxSize, int ioPriority, DFile::ReadQCallbackFunc func, void *userData)
{
    GInputStream *inputStream = d->inputStream();
    if (!inputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(QByteArray(), userData);
        return;
    }

    char data[maxSize + 1];
    memset(&data, 0, maxSize + 1);

    DFilePrivate::ReadQAsyncOp *dataOp = g_new0(DFilePrivate::ReadQAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;
    dataOp->data = data;

    d->checkAndResetCancel();
    g_input_stream_read_async(inputStream,
                              data,
                              static_cast<gsize>(maxSize),
                              ioPriority,
                              d->cancellable,
                              DFilePrivate::readQAsyncCallback,
                              dataOp);
}

void DFile::readAllAsync(int ioPriority, DFile::ReadAllCallbackFunc func, void *userData)
{
    GInputStream *inputStream = d->inputStream();
    if (!inputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(QByteArray(), userData);
        return;
    }

    const gsize size = 8192;

    char data[size + 1];
    memset(data, 0, size + 1);

    DFilePrivate::ReadAllAsyncOp *dataOp = g_new0(DFilePrivate::ReadAllAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;
    dataOp->data = data;
    dataOp->ioPriority = ioPriority;
    dataOp->me = d.data();

    d->checkAndResetCancel();
    g_input_stream_read_all_async(inputStream,
                                  data,
                                  size,
                                  ioPriority,
                                  d->cancellable,
                                  DFilePrivate::readAllAsyncCallback,
                                  dataOp);
}

void DFile::writeAsync(const char *data, qint64 maxSize, int ioPriority, DFile::WriteCallbackFunc func, void *userData)
{
    GOutputStream *outputStream = d->outputStream();
    if (!outputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(-1, userData);
        return;
    }

    DFilePrivate::WriteAsyncOp *dataOp = g_new0(DFilePrivate::WriteAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;

    d->checkAndResetCancel();
    g_output_stream_write_async(outputStream,
                                data,
                                static_cast<gsize>(maxSize),
                                ioPriority,
                                d->cancellable,
                                DFilePrivate::writeAsyncCallback,
                                dataOp);
}

void DFile::writeAllAsync(const char *data, int ioPriority, DFile::WriteAllCallbackFunc func, void *userData)
{
    writeAsync(data, strlen(data), ioPriority, func, userData);
}

void DFile::writeQAsync(const QByteArray &byteArray, int ioPriority, DFile::WriteQCallbackFunc func, void *userData)
{
    writeAsync(byteArray.data(), byteArray.length(), ioPriority, func, userData);
}

DFileFuture *DFile::openAsync(OpenFlags mode, int ioPriority, QObject *parent)
{
    Q_UNUSED(ioPriority);

    DFileFuture *future = new DFileFuture(parent);

    QPointer<DFilePrivate> me = d.data();
    QtConcurrent::run([&]() {
        this->open(mode);
        if (!me)
            return;
        future->finished();
    });
    return future;
}

DFileFuture *DFile::closeAsync(int ioPriority, QObject *parent)
{
    Q_UNUSED(ioPriority);

    DFileFuture *future = new DFileFuture(parent);

    QPointer<DFilePrivate> me = d.data();
    QtConcurrent::run([&]() {
        this->close();
        if (!me)
            return;
        future->finished();
    });
    return future;
}

DFileFuture *DFile::readAsync(quint64 maxSize, int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    GInputStream *inputStream = d->inputStream();
    if (!inputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return future;
    }

    QByteArray data;
    DFilePrivate::ReadAllAsyncFutureOp *dataOp = g_new0(DFilePrivate::ReadAllAsyncFutureOp, 1);
    dataOp->me = d.data();
    dataOp->future = future;
    dataOp->data = data;

    d->checkAndResetCancel();
    g_input_stream_read_all_async(inputStream,
                                  &data,
                                  maxSize,
                                  ioPriority,
                                  d->cancellable,
                                  DFilePrivate::readAsyncFutureCallback,
                                  dataOp);
    return future;
}

DFileFuture *DFile::readAllAsync(int ioPriority, QObject *parent)
{
    return readAsync(G_MAXSSIZE, ioPriority, parent);
}

DFileFuture *DFile::writeAsync(const QByteArray &data, qint64 len, int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    GOutputStream *outputStream = d->outputStream();
    if (!outputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return future;
    }

    DFilePrivate::NormalFutureAsyncOp *dataOp = g_new0(DFilePrivate::NormalFutureAsyncOp, 1);
    dataOp->me = d.data();
    dataOp->future = future;

    d->checkAndResetCancel();
    g_output_stream_write_async(outputStream,
                                data,
                                static_cast<gsize>(len),
                                ioPriority,
                                d->cancellable,
                                DFilePrivate::writeAsyncFutureCallback,
                                dataOp);

    return future;
}

DFileFuture *DFile::writeAsync(const QByteArray &data, int ioPriority, QObject *parent)
{
    return writeAsync(data, strlen(data), ioPriority, parent);
}

DFileFuture *DFile::flushAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    GOutputStream *outputStream = d->outputStream();
    if (!outputStream) {
        d->error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return future;
    }

    DFilePrivate::NormalFutureAsyncOp *data = g_new0(DFilePrivate::NormalFutureAsyncOp, 1);
    data->me = d.data();
    data->future = future;

    d->checkAndResetCancel();
    g_output_stream_flush_async(outputStream, ioPriority, d->cancellable, d->flushAsyncCallback, data);

    return future;
}

DFileFuture *DFile::sizeAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    DFilePrivate::NormalFutureAsyncOp *data = g_new0(DFilePrivate::NormalFutureAsyncOp, 1);
    data->me = d.data();
    data->future = future;

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);
    d->checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kStandardSize);
    g_file_query_info_async(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, ioPriority, d->cancellable, DFilePrivate::sizeAsyncCallback, data);

    return future;
}

DFileFuture *DFile::existsAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    DFilePrivate::NormalFutureAsyncOp *data = g_new0(DFilePrivate::NormalFutureAsyncOp, 1);
    data->me = d.data();
    data->future = future;

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);
    d->checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kStandardType);
    g_file_query_info_async(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, ioPriority, d->cancellable, d->existsAsyncCallback, data);

    return future;
}

DFileFuture *DFile::permissionsAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    DFilePrivate::NormalFutureAsyncOp *data = g_new0(DFilePrivate::NormalFutureAsyncOp, 1);
    data->me = d.data();
    data->future = future;

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);
    d->checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);
    g_file_query_info_async(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, ioPriority, d->cancellable, d->permissionsAsyncCallback, data);

    return future;
}

DFileFuture *DFile::setPermissionsAsync(Permissions permission, int ioPriority, QObject *parent)
{
    Q_UNUSED(ioPriority)

    DFileFuture *future = new DFileFuture(parent);

    quint32 stMode = d->buildPermissions(permission);
    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);
    d->checkAndResetCancel();
    g_autoptr(GError) gerror = nullptr;
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);

    QPointer<DFilePrivate> me = d.data();
    QtConcurrent::run([&]() {
        g_file_set_attribute_uint32(gfile, attributeKey.c_str(), stMode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, d->cancellable, &gerror);
        if (!me)
            return;
        if (gerror)
            d->setErrorFromGError(gerror);
        future->finished();
    });
    return future;
}
