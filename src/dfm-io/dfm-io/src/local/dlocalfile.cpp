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

#include "local/dlocalfile.h"
#include "local/dlocalfile_p.h"
#include "local/dlocalhelper.h"

#include "core/dfile_p.h"

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

    if (mode & DFile::OpenFlag::kReadOnly && !(mode & DFile::OpenFlag::kWriteOnly)) {
        if (!exists()) {
            return false;
        }
        iStream = (GInputStream *)g_file_read(gfile, nullptr, &gerror);
        if (gerror)
            setErrorFromGError(gerror);

        if (!iStream) {
            return false;
        }
        return true;
    } else if (mode & DFile::OpenFlag::kWriteOnly && !(mode & DFile::OpenFlag::kReadOnly)) {
        if (mode & DFile::OpenFlag::kNewOnly) {
            oStream = (GOutputStream *)g_file_create(gfile, G_FILE_CREATE_NONE, nullptr, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!oStream) {
                return false;
            }
        } else if (mode & DFile::OpenFlag::kAppend) {
            oStream = (GOutputStream *)g_file_append_to(gfile, G_FILE_CREATE_NONE, nullptr, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!oStream) {
                return false;
            }
        } else {
            oStream = (GOutputStream *)g_file_replace(gfile, nullptr, false, G_FILE_CREATE_NONE, nullptr, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!oStream) {
                return false;
            }
        }

        return true;
    } else if (mode & DFile::OpenFlag::kReadOnly && mode & DFile::OpenFlag::kWriteOnly) {
        if (mode & DFile::OpenFlag::kNewOnly) {
            ioStream = (GIOStream *)g_file_create_readwrite(gfile, G_FILE_CREATE_NONE, nullptr, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!ioStream) {
                return false;
            }
        } else if (mode & DFile::OpenFlag::kExistingOnly) {
            ioStream = (GIOStream *)g_file_open_readwrite(gfile, nullptr, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!ioStream) {
                return false;
            }
        } else {
            ioStream = (GIOStream *)g_file_replace_readwrite(gfile, nullptr, false, G_FILE_CREATE_NONE, nullptr, &gerror);
            if (gerror)
                setErrorFromGError(gerror);

            if (!ioStream) {
                return false;
            }
        }
        return true;
    } else {
        ioStream = (GIOStream *)g_file_replace_readwrite(gfile, nullptr, false, G_FILE_CREATE_NONE, nullptr, &gerror);
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
    gssize read = g_input_stream_read(inputStream,
                                      data,
                                      static_cast<gsize>(maxSize),
                                      nullptr,
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
    g_input_stream_read(inputStream,
                        data,
                        static_cast<gsize>(maxSize),
                        nullptr,
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

        gboolean read = g_input_stream_read_all(inputStream,
                                                data,
                                                size,
                                                &bytesRead,
                                                nullptr,
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

        dataRet.append(data, size);
    }

    dataRet.append('\0');
    return dataRet;
}

typedef struct
{
    DFile::ReadCallbackFunc callback;
    gpointer user_data;
} ReadAsyncOp;

void ReadAsyncCallback(GObject *source_object,
                       GAsyncResult *res,
                       gpointer user_data)
{
    ReadAsyncOp *data = static_cast<ReadAsyncOp *>(user_data);
    GInputStream *stream = (GInputStream *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    gssize size = g_input_stream_read_finish(stream, res, &gerror);
    if (data->callback)
        data->callback(size, data->user_data);
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
    dataOp->user_data = userData;

    g_input_stream_read_async(inputStream,
                              data,
                              static_cast<gsize>(maxSize),
                              ioPriority,
                              nullptr,
                              ReadAsyncCallback,
                              dataOp);
}

typedef struct
{
    DFile::ReadQCallbackFunc callback;
    char *data;
    gpointer user_data;
} ReadQAsyncOp;

void ReadQAsyncCallback(GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
    ReadQAsyncOp *data = static_cast<ReadQAsyncOp *>(user_data);
    GInputStream *stream = (GInputStream *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    gssize size = g_input_stream_read_finish(stream, res, &gerror);
    QByteArray dataRet = size >= 0 ? QByteArray(data->data) : QByteArray();
    if (data->callback)
        data->callback(dataRet, data->user_data);
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
    dataOp->user_data = userData;
    dataOp->data = data;

    g_input_stream_read_async(inputStream,
                              data,
                              static_cast<gsize>(maxSize),
                              ioPriority,
                              nullptr,
                              ReadQAsyncCallback,
                              dataOp);
}

typedef struct
{
    char *data;
    int ioPriority;
    DFile::ReadAllCallbackFunc callback;
    gpointer user_data;
    QPointer<DLocalFilePrivate> me;

} ReadAllAsyncOp;

void ReadAllAsyncCallback(GObject *source_object,
                          GAsyncResult *res,
                          gpointer user_data)
{
    ReadAllAsyncOp *data = static_cast<ReadAllAsyncOp *>(user_data);
    GInputStream *stream = (GInputStream *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    gsize size = 0;
    bool succ = g_input_stream_read_all_finish(stream, res, &size, &gerror);
    if (!succ || gerror) {
        if (data->callback)
            data->callback(QByteArray(), data->user_data);
    }
    if (size == 0) {
        if (data->callback) {
            if (data->me)
                data->callback(data->me->readAllAsyncRet, data->user_data);
        }
    }

    if (data->me) {
        data->me->readAllAsyncRet.append(data->data);
        data->me->readAllAsync(data->ioPriority, data->callback, data->user_data);
    }
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
    dataOp->user_data = userData;
    dataOp->data = data;
    dataOp->ioPriority = ioPriority;
    dataOp->me = this;

    g_input_stream_read_all_async(inputStream,
                                  data,
                                  size,
                                  ioPriority,
                                  nullptr,
                                  ReadAllAsyncCallback,
                                  dataOp);
}

qint64 DLocalFilePrivate::write(const char *data, qint64 maxSize)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
        return -1;
    }

    GError *gerror = nullptr;
    gssize write = g_output_stream_write(outputStream,
                                         data,
                                         static_cast<gsize>(maxSize),
                                         nullptr,
                                         &gerror);
    if (gerror) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
    }
    return write;
}

qint64 DLocalFilePrivate::write(const char *data)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_OPEN_FAILED);
    }

    gsize bytes_write;
    GError *gerror = nullptr;
    gssize write = g_output_stream_write_all(outputStream,
                                             data,
                                             strlen(data),
                                             &bytes_write,
                                             nullptr,
                                             &gerror);
    if (gerror) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
    }
    return write;
}

qint64 DLocalFilePrivate::write(const QByteArray &data)
{
    return write(data.data());
}

typedef struct
{
    DFile::WriteCallbackFunc callback;
    gpointer user_data;
} WriteAsyncOp;

void WriteAsyncCallback(GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
    WriteAsyncOp *data = static_cast<WriteAsyncOp *>(user_data);
    GOutputStream *stream = (GOutputStream *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    gssize size = g_output_stream_write_finish(stream, res, &gerror);
    if (data->callback)
        data->callback(size, data->user_data);
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
    dataOp->user_data = userData;

    g_output_stream_write_async(outputStream,
                                data,
                                static_cast<gsize>(maxSize),
                                ioPriority,
                                nullptr,
                                WriteAsyncCallback,
                                dataOp);
}

void DLocalFilePrivate::writeAllAsync(const char *data, int ioPriority, DFile::WriteAllCallbackFunc func, void *userData)
{
    writeAsync(data, strlen(data), ioPriority, func, userData);
}

void DLocalFilePrivate::writeQAsync(const QByteArray &byteArray, int ioPriority, DFile::WriteQCallbackFunc func, void *userData)
{
    writeAllAsync(byteArray.data(), ioPriority, func, userData);
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

    ret = g_seekable_seek(seekable, pos, gtype, nullptr, &gerror);
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

    GError *gerror = nullptr;
    gboolean ret = g_output_stream_flush(outputStream, nullptr, &gerror);

    if (gerror) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
    }
    return ret;
}

qint64 DLocalFilePrivate::size()
{
    const QUrl &uri = q->uri();
    GFile *gfile = g_file_new_for_uri(uri.toString().toStdString().c_str());

    GError *gerror = nullptr;
    GFileInfo *fileInfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, nullptr, &gerror);

    if (gerror) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
    }

    if (fileInfo) {
        goffset size = g_file_info_get_size(fileInfo);
        g_object_unref(fileInfo);
        return qint64(size);
    }

    return -1;
}

bool DLocalFilePrivate::exists()
{
    const QUrl &&uri = q->uri();
    g_autoptr(GFile) gfile = g_file_new_for_uri(uri.toString().toLocal8Bit().data());
    return g_file_query_file_type(gfile, G_FILE_QUERY_INFO_NONE, nullptr) != G_FILE_TYPE_UNKNOWN;
}

DFile::Permissions DLocalFilePrivate::permissions()
{
    DFile::Permissions retValue = DFile::Permission::kNoPermission;
    // 获取系统默认权限
    const QUrl &&url = q->uri();
    const QByteArray &path = url.toLocalFile().toLocal8Bit();

    struct stat buf;
    stat(path.data(), &buf);

    if ((buf.st_mode & S_IXUSR) == S_IXUSR) {
        retValue |= DFile::Permission::kExeOwner;
        retValue |= DFile::Permission::kExeUser;
    }
    if ((buf.st_mode & S_IWUSR) == S_IWUSR) {
        retValue |= DFile::Permission::kWriteOwner;
        retValue |= DFile::Permission::kWriteUser;
    }
    if ((buf.st_mode & S_IRUSR) == S_IRUSR) {
        retValue |= DFile::Permission::kReadOwner;
        retValue |= DFile::Permission::kReadUser;
    }

    if ((buf.st_mode & S_IXGRP) == S_IXGRP)
        retValue |= DFile::Permission::kExeGroup;
    if ((buf.st_mode & S_IWGRP) == S_IWGRP)
        retValue |= DFile::Permission::kWriteGroup;
    if ((buf.st_mode & S_IRGRP) == S_IRGRP)
        retValue |= DFile::Permission::kReadGroup;

    if ((buf.st_mode & S_IXOTH) == S_IXOTH)
        retValue |= DFile::Permission::kExeOther;
    if ((buf.st_mode & S_IWOTH) == S_IWOTH)
        retValue |= DFile::Permission::kWriteOther;
    if ((buf.st_mode & S_IROTH) == S_IROTH)
        retValue |= DFile::Permission::kReadOther;

    return retValue;
}

bool DLocalFilePrivate::setPermissions(DFile::Permissions permission)
{
    const QUrl &&url = q->uri();
    const QByteArray &path = url.toLocalFile().toLocal8Bit();

    struct stat buf;
    buf.st_mode = 0000;
    if (permission.testFlag(DFile::Permission::kExeOwner) | permission.testFlag(DFile::Permission::kExeUser))
        buf.st_mode |= S_IXUSR;
    if (permission.testFlag(DFile::Permission::kWriteOwner) | permission.testFlag(DFile::Permission::kWriteUser))
        buf.st_mode |= S_IWUSR;
    if (permission.testFlag(DFile::Permission::kReadOwner) | permission.testFlag(DFile::Permission::kReadUser))
        buf.st_mode |= S_IRUSR;

    if (permission.testFlag(DFile::Permission::kExeGroup))
        buf.st_mode |= S_IXGRP;
    if (permission.testFlag(DFile::Permission::kWriteGroup))
        buf.st_mode |= S_IWGRP;
    if (permission.testFlag(DFile::Permission::kReadGroup))
        buf.st_mode |= S_IRGRP;

    if (permission.testFlag(DFile::Permission::kExeOther))
        buf.st_mode |= S_IXOTH;
    if (permission.testFlag(DFile::Permission::kWriteOther))
        buf.st_mode |= S_IWOTH;
    if (permission.testFlag(DFile::Permission::kReadOther))
        buf.st_mode |= S_IROTH;

    return ::chmod(path.data(), buf.st_mode) == 0;
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
    error.setCode(DFMIOErrorCode(gerror->code));
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

void DLocalFilePrivate::freeCancellable(GCancellable *gcancellable)
{
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
    registerReadAsync(bind_field(this, &DLocalFile::readAsync));
    registerReadQAsync(bind_field(this, &DLocalFile::readQAsync));
    registerReadAllAsync(bind_field(this, &DLocalFile::readAllAsync));

    registerWrite(std::bind<bind_write>(&DLocalFile::write, this, std::placeholders::_1, std::placeholders::_2));
    registerWriteAll(std::bind<bind_writeAll>(&DLocalFile::write, this, std::placeholders::_1));
    registerWriteQ(std::bind<bind_writeQ>(&DLocalFile::write, this, std::placeholders::_1));
    // async
    registerWriteAsync(bind_field(this, &DLocalFile::writeAsync));
    registerWriteAllAsync(bind_field(this, &DLocalFile::writeAllAsync));
    registerWriteQAsync(bind_field(this, &DLocalFile::writeQAsync));

    registerSeek(std::bind(&DLocalFile::seek, this, std::placeholders::_1, std::placeholders::_2));
    registerPos(std::bind(&DLocalFile::pos, this));
    registerFlush(std::bind(&DLocalFile::flush, this));
    registerSize(std::bind(&DLocalFile::size, this));
    registerExists(std::bind(&DLocalFile::exists, this));
    registerPermissions(std::bind(&DLocalFile::permissions, this));
    registerSetPermissions(std::bind(&DLocalFile::setPermissions, this, std::placeholders::_1));

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

bool DLocalFile::seek(qint64 pos, DFile::SeekType type)
{
    return d->seek(pos, type);
}

qint64 DLocalFile::pos()
{
    return d->pos();
}

bool DLocalFile::flush()
{
    return d->flush();
}

qint64 DLocalFile::size()
{
    return d->size();
}

bool DLocalFile::exists()
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

void DLocalFile::setError(DFMIOError error)
{
    d->setError(error);
}

DFMIOError DLocalFile::lastError() const
{
    return d->lastError();
}
