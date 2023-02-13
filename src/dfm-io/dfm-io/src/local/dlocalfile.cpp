// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "local/dlocalfile.h"
#include "local/dlocalfile_p.h"
#include "local/dlocalhelper.h"
#include "core/dfilefuture.h"

#include "core/dfile_p.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QPointer>
#include <QDebug>

#include <sys/stat.h>

USING_IO_NAMESPACE

DLocalFilePrivate::DLocalFilePrivate(DLocalFile *q)
    : q(q)
{
}

DLocalFilePrivate::~DLocalFilePrivate()
{
}

bool DLocalFilePrivate::open(DFile::OpenFlags mode)
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
    g_autoptr(GFile) gfile = g_file_new_for_uri(uri.toString().toLocal8Bit().data());
    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();

    if (mode & DFile::OpenFlag::kReadOnly && !(mode & DFile::OpenFlag::kWriteOnly)) {
        if (!exists()) {
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

bool DLocalFilePrivate::close()
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

qint64 DLocalFilePrivate::read(char *data, qint64 maxSize)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return -1;
    }

    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    gssize read = g_input_stream_read(inputStream,
                                      data,
                                      static_cast<gsize>(maxSize),
                                      cancellable,
                                      &gerror);

    if (gerror) {
        setErrorFromGError(gerror);
        return -1;
    }

    return read;
}

QByteArray DLocalFilePrivate::read(qint64 maxSize)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return QByteArray();
    }

    char data[maxSize + 1];
    memset(&data, 0, maxSize + 1);

    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    g_input_stream_read(inputStream,
                        data,
                        static_cast<gsize>(maxSize),
                        cancellable,
                        &gerror);
    if (gerror) {
        setErrorFromGError(gerror);
        return QByteArray();
    }

    return QByteArray(data);
}

QByteArray DLocalFilePrivate::readAll()
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
        char data[size + 1];
        memset(data, 0, size + 1);

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

void DLocalFilePrivate::readAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
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

void DLocalFilePrivate::readAsync(char *data, qint64 maxSize, int ioPriority, DFile::ReadCallbackFunc func, void *userData)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(-1, userData);
        return;
    }

    ReadAsyncOp *dataOp = g_new0(ReadAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;

    checkAndResetCancel();
    g_input_stream_read_async(inputStream,
                              data,
                              static_cast<gsize>(maxSize),
                              ioPriority,
                              cancellable,
                              readAsyncCallback,
                              dataOp);
}

void DLocalFilePrivate::readQAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
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

void DLocalFilePrivate::readQAsync(qint64 maxSize, int ioPriority, DFile::ReadQCallbackFunc func, void *userData)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(QByteArray(), userData);
        return;
    }

    char data[maxSize + 1];
    memset(&data, 0, maxSize + 1);

    ReadQAsyncOp *dataOp = g_new0(ReadQAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;
    dataOp->data = data;

    checkAndResetCancel();
    g_input_stream_read_async(inputStream,
                              data,
                              static_cast<gsize>(maxSize),
                              ioPriority,
                              cancellable,
                              readQAsyncCallback,
                              dataOp);
}

void DLocalFilePrivate::readAllAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
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
        data->me->readAllAsync(data->ioPriority, data->callback, data->userData);
    }

    data->callback = nullptr;
    data->userData = nullptr;
    data->data = nullptr;
    data->ioPriority = 0;
    data->me = nullptr;
    g_free(data);
}

void DLocalFilePrivate::readAllAsync(int ioPriority, DFile::ReadAllCallbackFunc func, void *userData)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(QByteArray(), userData);
        return;
    }

    const gsize size = 8192;

    char data[size + 1];
    memset(data, 0, size + 1);

    ReadAllAsyncOp *dataOp = g_new0(ReadAllAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;
    dataOp->data = data;
    dataOp->ioPriority = ioPriority;
    dataOp->me = this;

    checkAndResetCancel();
    g_input_stream_read_all_async(inputStream,
                                  data,
                                  size,
                                  ioPriority,
                                  cancellable,
                                  readAllAsyncCallback,
                                  dataOp);
}

qint64 DLocalFilePrivate::write(const char *data, qint64 maxSize)
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
    if (gerror)
        setErrorFromGError(gerror);

    return write;
}

qint64 DLocalFilePrivate::write(const char *data)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
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
    if (gerror)
        setErrorFromGError(gerror);

    return write;
}

qint64 DLocalFilePrivate::write(const QByteArray &data)
{
    return write(data.data(), data.length());
}

void DLocalFilePrivate::writeAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
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

void DLocalFilePrivate::permissionsAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    if (!data)
        return;

    QPointer<DLocalFilePrivate> me = data->me;
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

void DLocalFilePrivate::existsAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    if (!data)
        return;

    QPointer<DLocalFilePrivate> me = data->me;
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

void DLocalFilePrivate::sizeAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    if (!data)
        return;

    QPointer<DLocalFilePrivate> me = data->me;
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

void DLocalFilePrivate::flushAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    QPointer<DLocalFilePrivate> me = data->me;
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

void DLocalFilePrivate::writeAsyncFutureCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    NormalFutureAsyncOp *data = static_cast<NormalFutureAsyncOp *>(userData);
    QPointer<DLocalFilePrivate> me = data->me;
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

void DLocalFilePrivate::readAsyncFutureCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    ReadAllAsyncFutureOp *data = static_cast<ReadAllAsyncFutureOp *>(userData);
    GInputStream *stream = (GInputStream *)(sourceObject);
    QPointer<DLocalFilePrivate> me = data->me;
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

void DLocalFilePrivate::checkAndResetCancel()
{
    if (cancellable) {
        g_object_unref(cancellable);
        cancellable = nullptr;
    }
    cancellable = g_cancellable_new();
}

void DLocalFilePrivate::writeAsync(const char *data, qint64 maxSize, int ioPriority, DFile::WriteCallbackFunc func, void *userData)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        if (func)
            func(-1, userData);
        return;
    }

    WriteAsyncOp *dataOp = g_new0(WriteAsyncOp, 1);
    dataOp->callback = func;
    dataOp->userData = userData;

    checkAndResetCancel();
    g_output_stream_write_async(outputStream,
                                data,
                                static_cast<gsize>(maxSize),
                                ioPriority,
                                cancellable,
                                writeAsyncCallback,
                                dataOp);
}

void DLocalFilePrivate::writeAllAsync(const char *data, int ioPriority, DFile::WriteAllCallbackFunc func, void *userData)
{
    writeAsync(data, strlen(data), ioPriority, func, userData);
}

void DLocalFilePrivate::writeQAsync(const QByteArray &byteArray, int ioPriority, DFile::WriteQCallbackFunc func, void *userData)
{
    writeAsync(byteArray.data(), byteArray.length(), ioPriority, func, userData);
}

bool DLocalFilePrivate::cancel()
{
    if (cancellable && !g_cancellable_is_cancelled(cancellable))
        g_cancellable_cancel(cancellable);
    return true;
}

bool DLocalFilePrivate::seek(qint64 pos, DFile::SeekType type)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return -1;
    }

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

    checkAndResetCancel();
    ret = g_seekable_seek(seekable, pos, gtype, cancellable, &gerror);
    if (gerror) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
    }

    return ret;
}

qint64 DLocalFilePrivate::pos()
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return -1;
    }

    // seems g_seekable_can_seek only support local file, survey after. todo lanxs
    gboolean canSeek = G_IS_SEEKABLE(inputStream) /*&& g_seekable_can_seek(G_SEEKABLE(inputStream))*/;
    if (!canSeek) {
        return false;
    }

    GSeekable *seekable = G_SEEKABLE(inputStream);
    if (!seekable) {
        return false;
    }

    goffset pos = g_seekable_tell(seekable);

    return qint64(pos);
}

bool DLocalFilePrivate::flush()
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return false;
    }

    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    gboolean ret = g_output_stream_flush(outputStream, cancellable, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    return ret;
}

qint64 DLocalFilePrivate::size()
{
    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = g_file_new_for_uri(uri.toString().toStdString().c_str());

    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    g_autoptr(GFileInfo) fileInfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, cancellable, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    if (fileInfo) {
        goffset size = g_file_info_get_size(fileInfo);
        return qint64(size);
    }

    return -1;
}

bool DLocalFilePrivate::exists()
{
    const QUrl &&uri = q->uri();
    g_autoptr(GFile) gfile = g_file_new_for_uri(uri.toString().toLocal8Bit().data());
    checkAndResetCancel();
    return g_file_query_file_type(gfile, G_FILE_QUERY_INFO_NONE, cancellable) != G_FILE_TYPE_UNKNOWN;
}

DFile::Permissions DLocalFilePrivate::permissions()
{
    DFile::Permissions retValue = DFile::Permission::kNoPermission;

    g_autoptr(GFile) gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());

    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);
    if (attributeKey.empty())
        return retValue;
    g_autoptr(GFileInfo) fileInfo = g_file_query_info(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, cancellable, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    if (!fileInfo)
        return retValue;

    return permissionsFromGFileInfo(fileInfo);
}

bool DLocalFilePrivate::setPermissions(DFile::Permissions permission)
{
    quint32 stMode = buildPermissions(permission);

    g_autoptr(GFile) gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());
    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);
    bool succ = g_file_set_attribute_uint32(gfile, attributeKey.c_str(), stMode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, cancellable, &gerror);
    if (gerror)
        setErrorFromGError(gerror);
    return succ;
}

quint32 DLocalFilePrivate::buildPermissions(DFile::Permissions permission)
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

DFile::Permissions DLocalFilePrivate::permissionsFromGFileInfo(GFileInfo *gfileinfo)
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

DFileFuture *DLocalFilePrivate::openAsync(DFile::OpenFlags mode, int ioPriority, QObject *parent)
{
    Q_UNUSED(ioPriority);

    DFileFuture *future = new DFileFuture(parent);

    QPointer<DLocalFilePrivate> me = this;
    QtConcurrent::run([&]() {
        this->open(mode);
        if (!me)
            return;
        future->finished();
    });
    return future;
}

DFileFuture *DLocalFilePrivate::closeAsync(int ioPriority, QObject *parent)
{
    Q_UNUSED(ioPriority);

    DFileFuture *future = new DFileFuture(parent);

    QPointer<DLocalFilePrivate> me = this;
    QtConcurrent::run([&]() {
        this->close();
        if (!me)
            return;
        future->finished();
    });
    return future;
}

DFileFuture *DLocalFilePrivate::readAsync(qint64 maxSize, int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return future;
    }

    QByteArray data;
    ReadAllAsyncFutureOp *dataOp = g_new0(ReadAllAsyncFutureOp, 1);
    dataOp->me = this;
    dataOp->future = future;
    dataOp->data = data;

    checkAndResetCancel();
    g_input_stream_read_all_async(inputStream,
                                  &data,
                                  maxSize,
                                  ioPriority,
                                  cancellable,
                                  readAsyncFutureCallback,
                                  dataOp);
    return future;
}

DFileFuture *DLocalFilePrivate::readAllAsync(int ioPriority, QObject *parent)
{
    return readAsync(G_MAXSSIZE, ioPriority, parent);
}

DFileFuture *DLocalFilePrivate::writeAsync(const QByteArray &data, qint64 len, int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return future;
    }

    NormalFutureAsyncOp *dataOp = g_new0(NormalFutureAsyncOp, 1);
    dataOp->me = this;
    dataOp->future = future;

    checkAndResetCancel();
    g_output_stream_write_async(outputStream,
                                data,
                                static_cast<gsize>(len),
                                ioPriority,
                                cancellable,
                                writeAsyncFutureCallback,
                                dataOp);

    return future;
}

DFileFuture *DLocalFilePrivate::writeAsync(const QByteArray &data, int ioPriority, QObject *parent)
{
    return writeAsync(data, strlen(data), ioPriority, parent);
}

DFileFuture *DLocalFilePrivate::flushAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return future;
    }

    NormalFutureAsyncOp *data = g_new0(NormalFutureAsyncOp, 1);
    data->me = this;
    data->future = future;

    checkAndResetCancel();
    g_output_stream_flush_async(outputStream, ioPriority, cancellable, flushAsyncCallback, data);

    return future;
}

DFileFuture *DLocalFilePrivate::sizeAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    NormalFutureAsyncOp *data = g_new0(NormalFutureAsyncOp, 1);
    data->me = this;
    data->future = future;

    g_autoptr(GFile) gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());
    checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kStandardSize);
    g_file_query_info_async(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, ioPriority, cancellable, sizeAsyncCallback, data);

    return future;
}

DFileFuture *DLocalFilePrivate::existsAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    NormalFutureAsyncOp *data = g_new0(NormalFutureAsyncOp, 1);
    data->me = this;
    data->future = future;

    g_autoptr(GFile) gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());
    checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kStandardType);
    g_file_query_info_async(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, ioPriority, cancellable, existsAsyncCallback, data);

    return future;
}

DFileFuture *DLocalFilePrivate::permissionsAsync(int ioPriority, QObject *parent)
{
    DFileFuture *future = new DFileFuture(parent);

    NormalFutureAsyncOp *data = g_new0(NormalFutureAsyncOp, 1);
    data->me = this;
    data->future = future;

    g_autoptr(GFile) gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());
    checkAndResetCancel();
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);
    g_file_query_info_async(gfile, attributeKey.c_str(), G_FILE_QUERY_INFO_NONE, ioPriority, cancellable, permissionsAsyncCallback, data);

    return future;
}

DFileFuture *DLocalFilePrivate::setPermissionsAsync(DFile::Permissions permission, int ioPriority, QObject *parent)
{
    Q_UNUSED(ioPriority)

    DFileFuture *future = new DFileFuture(parent);

    quint32 stMode = buildPermissions(permission);
    g_autoptr(GFile) gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());
    checkAndResetCancel();
    g_autoptr(GError) gerror = nullptr;
    const std::string &attributeKey = DLocalHelper::attributeStringById(DFileInfo::AttributeID::kUnixMode);

    QPointer<DLocalFilePrivate> me = this;
    QtConcurrent::run([&]() {
        g_file_set_attribute_uint32(gfile, attributeKey.c_str(), stMode, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, cancellable, &gerror);
        if (!me)
            return;
        if (gerror)
            setErrorFromGError(gerror);
        future->finished();
    });
    return future;
}

DFMIOError DLocalFilePrivate::lastError()
{
    return error;
}

void DLocalFilePrivate::setError(DFMIOError error)
{
    this->error = error;
}

void DLocalFilePrivate::setErrorFromGError(GError *gerror)
{
    if (!gerror)
        return;
    error.setCode(DFMIOErrorCode(gerror->code));
    if (gerror->domain != G_IO_ERROR) {
        error.setCode(DFMIOErrorCode::DFM_ERROR_OTHER_DOMAIN);
        error.setMessage(gerror->message);
    }
}

bool DLocalFilePrivate::checkOpenFlags(DFile::OpenFlags *modeIn)
{
    DFile::OpenFlags &mode = *modeIn;

    if (mode & DFile::OpenFlag::kNewOnly) {
        if (exists()) {
            error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FLAG_ERROR);
            return false;
        }
    }
    if (mode & DFile::OpenFlag::kExistingOnly) {
        if (!exists()) {
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

GInputStream *DLocalFilePrivate::inputStream()
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

GOutputStream *DLocalFilePrivate::outputStream()
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

DLocalFile::DLocalFile(const QUrl &uri)
    : DFile(uri), d(new DLocalFilePrivate(this))
{
    using namespace std::placeholders;

    using bind_read = qint64 (DLocalFile::*)(char *, qint64);
    using bind_readQ = QByteArray (DLocalFile::*)(qint64);

    using bind_write = qint64 (DLocalFile::*)(const char *, qint64);
    using bind_writeAll = qint64 (DLocalFile::*)(const char *);
    using bind_writeQ = qint64 (DLocalFile::*)(const QByteArray &);

    registerOpen(std::bind(&DLocalFile::open, this, std::placeholders::_1));
    registerClose(std::bind(&DLocalFile::close, this));

    registerRead(std::bind<bind_read>(&DLocalFile::read, this, std::placeholders::_1, std::placeholders::_2));
    registerReadQ(std::bind<bind_readQ>(&DLocalFile::read, this, std::placeholders::_1));
    registerReadAll(std::bind(&DLocalFile::readAll, this));
    // async
    using funcReadAsync = void (DLocalFile::*)(char *, qint64, int, DFile::ReadCallbackFunc, void *);
    registerReadAsync(std::bind((funcReadAsync)&DLocalFile::readAsync, this, _1, _2, _3, _4, _5));
    registerReadQAsync(bind_field(this, &DLocalFile::readQAsync));
    using funcReadAllAsync = void (DLocalFile::*)(int, DFile::ReadAllCallbackFunc, void *);
    registerReadAllAsync(std::bind((funcReadAllAsync)&DLocalFile::readAllAsync, this, _1, _2, _3));

    registerWrite(std::bind<bind_write>(&DLocalFile::write, this, std::placeholders::_1, std::placeholders::_2));
    registerWriteAll(std::bind<bind_writeAll>(&DLocalFile::write, this, std::placeholders::_1));
    registerWriteQ(std::bind<bind_writeQ>(&DLocalFile::write, this, std::placeholders::_1));
    // async
    using funcWriteAsync = void (DLocalFile::*)(const char *, qint64, int, DFile::WriteCallbackFunc, void *);
    registerWriteAsync(std::bind((funcWriteAsync)&DLocalFile::writeAsync, this, _1, _2, _3, _4, _5));
    registerWriteAllAsync(bind_field(this, &DLocalFile::writeAllAsync));
    registerWriteQAsync(bind_field(this, &DLocalFile::writeQAsync));

    registerCancel(std::bind(&DLocalFile::cancel, this));
    registerSeek(std::bind(&DLocalFile::seek, this, std::placeholders::_1, std::placeholders::_2));
    registerPos(std::bind(&DLocalFile::pos, this));
    registerFlush(std::bind(&DLocalFile::flush, this));
    registerSize(std::bind(&DLocalFile::size, this));
    registerExists(std::bind(&DLocalFile::exists, this));
    registerPermissions(std::bind(&DLocalFile::permissions, this));
    registerSetPermissions(std::bind(&DLocalFile::setPermissions, this, std::placeholders::_1));

    // future
    registerOpenAsyncFuture(std::bind(&DLocalFile::openAsync, this, _1, _2, _3));
    registerCloseAsyncFuture(std::bind(&DLocalFile::closeAsync, this, _1, _2));
    using funcReadAsyncFuture = DFileFuture *(DLocalFile::*)(qint64, int, QObject *);
    registerReadAsyncFuture(std::bind((funcReadAsyncFuture)&DLocalFile::readAsync, this, _1, _2, _3));
    using funcReadAllAsyncFuture = DFileFuture *(DLocalFile::*)(int, QObject *);
    registerReadAllAsyncFuture(std::bind((funcReadAllAsyncFuture)&DLocalFile::readAllAsync, this, _1, _2));
    using funcWriteAsyncFuture = DFileFuture *(DLocalFile::*)(const QByteArray &, qint64, int, QObject *);
    registerWriteAsyncFuture(std::bind((funcWriteAsyncFuture)&DLocalFile::writeAsync, this, _1, _2, _3, _4));
    using funcWriteAllAsyncFuture = DFileFuture *(DLocalFile::*)(const QByteArray &, int, QObject *);
    registerWriteAllAsyncFuture(std::bind((funcWriteAllAsyncFuture)&DLocalFile::writeAsync, this, _1, _2, _3));
    registerFlushAsyncFuture(std::bind(&DLocalFile::flushAsync, this, _1, _2));
    registerSizeAsyncFuture(std::bind(&DLocalFile::sizeAsync, this, _1, _2));
    registerExistsAsyncFuture(std::bind(&DLocalFile::existsAsync, this, _1, _2));
    registerPermissionsAsyncFuture(std::bind(&DLocalFile::permissionsAsync, this, _1, _2));
    registerSetPermissionsAsyncFuture(std::bind(&DLocalFile::setPermissionsAsync, this, _1, _2, _3));

    registerSetError(std::bind(&DLocalFile::setError, this, std::placeholders::_1));
    registerLastError(std::bind(&DLocalFile::lastError, this));
}

DLocalFile::~DLocalFile()
{
    close();
}

bool DLocalFile::open(DFile::OpenFlags mode)
{
    return d->open(mode);
}

bool DLocalFile::close()
{
    return d->close();
}

qint64 DLocalFile::read(char *data, qint64 maxSize)
{
    return d->read(data, maxSize);
}

QByteArray DLocalFile::read(qint64 maxSize)
{
    return d->read(maxSize);
}

QByteArray DLocalFile::readAll()
{
    return d->readAll();
}

void DLocalFile::readAsync(char *data, qint64 maxSize, int ioPriority, DFile::ReadCallbackFunc func, void *userData)
{
    d->readAsync(data, maxSize, ioPriority, func, userData);
}

void DLocalFile::readQAsync(qint64 maxSize, int ioPriority, DFile::ReadQCallbackFunc func, void *userData)
{
    d->readQAsync(maxSize, ioPriority, func, userData);
}

void DLocalFile::readAllAsync(int ioPriority, DFile::ReadAllCallbackFunc func, void *userData)
{
    d->readAllAsync(ioPriority, func, userData);
}

qint64 DLocalFile::write(const char *data, qint64 len)
{
    return d->write(data, len);
}

qint64 DLocalFile::write(const char *data)
{
    return d->write(data);
}

qint64 DLocalFile::write(const QByteArray &byteArray)
{
    return d->write(byteArray);
}

void DLocalFile::writeAsync(const char *data, qint64 len, int ioPriority, DFile::WriteCallbackFunc func, void *userData)
{
    d->writeAsync(data, len, ioPriority, func, userData);
}

void DLocalFile::writeAllAsync(const char *data, int ioPriority, DFile::WriteAllCallbackFunc func, void *userData)
{
    d->writeAllAsync(data, ioPriority, func, userData);
}

void DLocalFile::writeQAsync(const QByteArray &byteArray, int ioPriority, DFile::WriteQCallbackFunc func, void *userData)
{
    d->writeQAsync(byteArray, ioPriority, func, userData);
}

bool DLocalFile::cancel() const
{
    return d->cancel();
}

bool DLocalFile::seek(qint64 pos, DFile::SeekType type) const
{
    return d->seek(pos, type);
}

qint64 DLocalFile::pos() const
{
    return d->pos();
}

bool DLocalFile::flush()
{
    return d->flush();
}

qint64 DLocalFile::size() const
{
    return d->size();
}

bool DLocalFile::exists() const
{
    return d->exists();
}

DFile::Permissions DLocalFile::permissions() const
{
    return d->permissions();
}

bool DLocalFile::setPermissions(DFile::Permissions permission)
{
    return d->setPermissions(permission);
}

DFileFuture *DLocalFile::openAsync(DFile::OpenFlags mode, int ioPriority, QObject *parent)
{
    return d->openAsync(mode, ioPriority, parent);
}

DFileFuture *DLocalFile::closeAsync(int ioPriority, QObject *parent)
{
    return d->closeAsync(ioPriority, parent);
}

DFileFuture *DLocalFile::readAsync(qint64 maxSize, int ioPriority, QObject *parent)
{
    return d->readAsync(maxSize, ioPriority, parent);
}

DFileFuture *DLocalFile::readAllAsync(int ioPriority, QObject *parent)
{
    return d->readAllAsync(ioPriority, parent);
}

DFileFuture *DLocalFile::writeAsync(const QByteArray &data, qint64 len, int ioPriority, QObject *parent)
{
    return d->writeAsync(data, len, ioPriority, parent);
}

DFileFuture *DLocalFile::writeAsync(const QByteArray &data, int ioPriority, QObject *parent)
{
    return d->writeAsync(data, ioPriority, parent);
}

DFileFuture *DLocalFile::flushAsync(int ioPriority, QObject *parent)
{
    return d->flushAsync(ioPriority, parent);
}

DFileFuture *DLocalFile::sizeAsync(int ioPriority, QObject *parent)
{
    return d->sizeAsync(ioPriority, parent);
}

DFileFuture *DLocalFile::existsAsync(int ioPriority, QObject *parent)
{
    return d->existsAsync(ioPriority, parent);
}

DFileFuture *DLocalFile::permissionsAsync(int ioPriority, QObject *parent)
{
    return d->permissionsAsync(ioPriority, parent);
}

DFileFuture *DLocalFile::setPermissionsAsync(DFile::Permissions permission, int ioPriority, QObject *parent)
{
    return d->setPermissionsAsync(permission, ioPriority, parent);
}

void DLocalFile::setError(DFMIOError error)
{
    d->setError(error);
}

DFMIOError DLocalFile::lastError() const
{
    return d->lastError();
}
