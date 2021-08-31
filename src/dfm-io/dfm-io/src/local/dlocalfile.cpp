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

#include "core/dfile_p.h"

#include <QDebug>

USING_IO_NAMESPACE

DLocalFilePrivate::DLocalFilePrivate(DLocalFile *ptr)
    : q_ptr(ptr)
{

}

DLocalFilePrivate::~DLocalFilePrivate()
{
}

bool DLocalFilePrivate::open(DFile::OpenFlag mode)
{
    Q_Q(DLocalFile);
    const QUrl &uri = q->uri();
    GFile *gfile = g_file_new_for_uri(uri.toString().toStdString().c_str());

    GError *error = nullptr;
    if (mode == DFile::OpenFlag::ReadOnly) {
        if (!exists()) {
            g_object_unref(gfile);
            return false;
        }
        ioStream = g_file_open_readwrite(gfile, nullptr, &error);
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
        }

        if (!ioStream) {
            g_object_unref(gfile);
            return false;
        }
    } else if (mode == DFile::OpenFlag::WriteOnly) {
        ioStream = g_file_replace_readwrite(gfile,
                                            nullptr,
                                            false,
                                            G_FILE_CREATE_NONE,
                                            nullptr,
                                            &error);
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
        }

        if (!ioStream) {
            g_object_unref(gfile);
            return false;
        }
    } else if (mode == DFile::OpenFlag::ReadWrite) {
        ioStream = g_file_replace_readwrite(gfile,
                                            nullptr,
                                            false,
                                            G_FILE_CREATE_NONE,
                                            nullptr,
                                            &error);
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
        }

        if (!ioStream) {
            g_object_unref(gfile);
            return false;
        }
    } else if (mode == DFile::OpenFlag::Append) {
        // 追加的方式打开
        oStream = (GOutputStream*)g_file_append_to(gfile, G_FILE_CREATE_NONE, nullptr, &error);
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
        }
        if (!oStream) {
            g_object_unref(gfile);
            return false;
        }

    } else if (mode == DFile::OpenFlag::Truncate) {
        // 覆盖的方式打开
        ioStream = g_file_replace_readwrite(gfile,
                                            nullptr,
                                            false,
                                            G_FILE_CREATE_NONE,
                                            nullptr,
                                            &error);
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
        }

        if (!ioStream) {
            g_object_unref(gfile);
            return false;
        }

    } else if (mode == DFile::OpenFlag::NewOnly) {
        // 仅新建方式打开，已经存在会报错
        if (exists()) {
            g_object_unref(gfile);
            return false;
        }
        ioStream = g_file_replace_readwrite(gfile,
                                            nullptr,
                                            false,
                                            G_FILE_CREATE_NONE,
                                            nullptr,
                                            &error);
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
        }

        if (!ioStream) {
            g_object_unref(gfile);
            return false;
        }

    } else if (mode == DFile::OpenFlag::ExistingOnly) {
        // 仅已存在的方式打开，不存在会报错
        if (!exists()) {
            g_object_unref(gfile);
            return false;
        }
        ioStream = g_file_replace_readwrite(gfile,
                                            nullptr,
                                            false,
                                            G_FILE_CREATE_NONE,
                                            nullptr,
                                            &error);
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
        }

        if (!ioStream) {
            g_object_unref(gfile);
            return false;
        }
    }

    g_object_unref(gfile);

    return true;
}

bool DLocalFilePrivate::close()
{
    if (ioStream) {
        g_io_stream_close((GIOStream*)ioStream, nullptr, nullptr);
        g_object_unref (ioStream);
        ioStream = nullptr;
    }
    if (iStream) {
        g_input_stream_close(iStream, nullptr, nullptr);
        g_object_unref (iStream);
        iStream = nullptr;
    }
    if (oStream) {
        g_output_stream_close(oStream, nullptr, nullptr);
        g_object_unref (oStream);
        oStream = nullptr;
    }
    return true;
}

qint64 DLocalFilePrivate::read(char *data, qint64 maxSize)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        qWarning() << "need input_stream before read.";
        return -1;
    }

    GError *error = nullptr;
    gssize read = g_input_stream_read(inputStream,
                                      data,
                                      static_cast<gsize>(maxSize),
                                      nullptr,
                                      &error);

    if (error) {
        qWarning() << error->message;
        g_error_free(error);
        return -1;
    }

    return read;
}

QByteArray DLocalFilePrivate::read(qint64 maxSize)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        qWarning() << "need input_stream before read.";
        return QByteArray();
    }

    char data[maxSize];
    GError *error = nullptr;
    gssize read = g_input_stream_read(inputStream,
                                      data,
                                      static_cast<gsize>(maxSize),
                                      nullptr,
                                      &error);
    Q_UNUSED(read);
    if (error) {
        qWarning() << error->message;
        g_error_free(error);
        return QByteArray();
    }

    return QByteArray(data);
}

QByteArray DLocalFilePrivate::readAll()
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        qWarning() << "need input_stream before read.";
        return QByteArray();
    }

    QByteArray dataRet;

    const uint16_t &size = 2048;

    gsize bytes_read;
    char data[size];
    GError *error = nullptr;

    while (true) {
        gboolean read = g_input_stream_read_all(inputStream,
                                          data,
                                          size,
                                          &bytes_read,
                                          nullptr,
                                          &error);
        if (!read || error) {
            if (error) {
                qWarning() << error->message;
                g_error_free(error);
            }
            break;
        }
        dataRet.append(data);
        memset(data, 0, size);
    }

    return dataRet;
}

qint64 DLocalFilePrivate::write(const char *data, qint64 maxSize)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        qWarning() << "need output_stream before read.";
        return -1;
    }

    GError *error = nullptr;
    gssize write = g_output_stream_write(outputStream,
                                         data,
                                         static_cast<gsize>(maxSize),
                                         nullptr,
                                         &error);
    if (error) {
        qWarning() << error->message;
        g_error_free(error);
    }
    return write;
}

qint64 DLocalFilePrivate::write(const char *data)
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        qWarning() << "need output_stream before read.";
        return -1;
    }

    gsize bytes_write;
    GError *error = nullptr;
    gssize write = g_output_stream_write_all(outputStream,
                                         data,
                                         strlen(data),
                                         &bytes_write,
                                         nullptr,
                                         &error);
    if (error) {
        qWarning() << error->message;
        g_error_free(error);
    }
    return write;
}

qint64 DLocalFilePrivate::write(const QByteArray &data)
{
    return write(data.data());
}

bool DLocalFilePrivate::seek(qint64 pos, DFile::DFMSeekType type)
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        qWarning() << "need input_stream before read.";
        return -1;
    }

    gboolean canSeek = G_IS_SEEKABLE (inputStream) && g_seekable_can_seek (G_SEEKABLE (inputStream));
    if (!canSeek) {
        qWarning() << "try seek failed.";
        return false;
    }

    GSeekable* seekable = G_SEEKABLE (inputStream);
    if (!seekable) {
        qWarning() << "try seek failed.";
        return false;
    }

    bool ret = false;
    GError *error = nullptr;
    GSeekType gtype = G_SEEK_CUR;
    switch (type)
    {
    case DFile::DFMSeekType::BEGIN:
        gtype = G_SEEK_SET;
        break;
    case DFile::DFMSeekType::END:
        gtype = G_SEEK_END;
        break;
    
    default:
        break;
    }

    ret = g_seekable_seek(seekable, pos, gtype, nullptr, &error);
    if (error) {
        g_error_free(error);
    }

    return ret;
}

qint64 DLocalFilePrivate::pos()
{
    GInputStream *inputStream = this->inputStream();
    if (!inputStream) {
        qWarning() << "need input_stream before read.";
        return -1;
    }

    gboolean canSeek = G_IS_SEEKABLE (inputStream) && g_seekable_can_seek (G_SEEKABLE (inputStream));
    if (!canSeek) {
        qWarning() << "try seek failed.";
        return false;
    }

    GSeekable* seekable = G_SEEKABLE (inputStream);
    if (!seekable) {
        qWarning() << "try seek failed.";
        return false;
    }

    goffset pos = g_seekable_tell(seekable);

    return qint64(pos);
}

bool DLocalFilePrivate::flush()
{
    GOutputStream *outputStream = this->outputStream();
    if (!outputStream) {
        qWarning() << "need output_stream before read.";
        return false;
    }

    GError *error = nullptr;
    gboolean ret = g_output_stream_flush(outputStream, nullptr, &error);

    if (error) {
        qWarning() << error->message;
        g_error_free(error);
    }
    return ret;
}

qint64 DLocalFilePrivate::size()
{
    Q_Q(DLocalFile);
    const QUrl &uri = q->uri();
    GFile *gfile = g_file_new_for_uri(uri.toString().toStdString().c_str());

    GError *error = nullptr;
    GFileInfo *fileInfo = g_file_query_info(gfile, "G_FILE_ATTRIBUTE_STANDARD_SIZE", G_FILE_QUERY_INFO_NONE, nullptr, &error);

    if (error) {
        qWarning() << error->message;
        g_error_free(error);
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
    Q_Q(DLocalFile);

    const QUrl &uri = q->uri();
    GFile *gfile = g_file_new_for_uri(uri.toString().toStdString().c_str());
    const bool exists = g_file_query_exists(gfile, nullptr);

    g_object_unref(gfile);
    return exists;
}

GInputStream *DLocalFilePrivate::inputStream()
{
    if (iStream)
        return iStream;

    if (ioStream) {
        GInputStream *inputStream = g_io_stream_get_input_stream((GIOStream*)ioStream);
        if (inputStream)
            return inputStream;
    }

    qWarning() << "get input stream failed.";

    return nullptr;
}

GOutputStream *DLocalFilePrivate::outputStream()
{
    if (oStream)
        return oStream;

    if (ioStream) {
        GOutputStream *outputStream = g_io_stream_get_output_stream((GIOStream*)ioStream);
        if (outputStream)
            return outputStream;
    }

    qWarning() << "get out stream failed.";

    return nullptr;
}

DLocalFile::DLocalFile(const QUrl &uri) : DFile(uri)
    , d_ptr(new DLocalFilePrivate(this))
{
    using bind_read = qint64(DLocalFile::*)(char *, qint64);
    using bind_readQ = QByteArray(DLocalFile::*)(qint64);
    using bind_readAll = QByteArray(DLocalFile::*)();

    using bind_write = qint64(DLocalFile::*)(const char *, qint64);
    using bind_writeAll = qint64(DLocalFile::*)(const char *);
    using bind_writeQ = qint64(DLocalFile::*)(const QByteArray &);

    registerOpen(std::bind(&DLocalFile::open, this, std::placeholders::_1));
    registerClose(std::bind(&DLocalFile::close, this));
    registerRead(std::bind((bind_read)&DLocalFile::read, this, std::placeholders::_1, std::placeholders::_2));
    registerReadQ(std::bind((bind_readQ)&DLocalFile::read, this, std::placeholders::_1));
    registerReadAll(std::bind((bind_readAll)&DLocalFile::readAll, this));
    registerWrite(std::bind((bind_write)&DLocalFile::write, this, std::placeholders::_1, std::placeholders::_2));
    registerWriteAll(std::bind((bind_writeAll)&DLocalFile::write, this, std::placeholders::_1));
    registerWriteQ(std::bind((bind_writeQ)&DLocalFile::write, this, std::placeholders::_1));
    registerSeek(std::bind(&DLocalFile::seek, this, std::placeholders::_1, std::placeholders::_2));
    registerPos(std::bind(&DLocalFile::pos, this));
    registerFlush(std::bind(&DLocalFile::flush, this));
    registerSize(std::bind(&DLocalFile::size, this));
    registerExists(std::bind(&DLocalFile::exists, this));
}

DLocalFile::~DLocalFile()
{
   close();
}

bool DLocalFile::open(DFile::OpenFlag mode)
{
    Q_D(DLocalFile);
    return d->open(mode);
}

bool DLocalFile::close()
{
    Q_D(DLocalFile);
    return d->close();
}

qint64 DLocalFile::read(char *data, qint64 maxSize)
{
    Q_D(DLocalFile);
    return d->read(data, maxSize);
}

QByteArray DLocalFile::read(qint64 maxSize)
{
    Q_D(DLocalFile);
    return d->read(maxSize);
}

QByteArray DLocalFile::readAll()
{
    Q_D(DLocalFile);
    return d->readAll();
}

qint64 DLocalFile::write(const char *data, qint64 len)
{
    Q_D(DLocalFile);
    return d->write(data, len);
}

qint64 DLocalFile::write(const char *data)
{
    Q_D(DLocalFile);
    return d->write(data);
}

qint64 DLocalFile::write(const QByteArray &byteArray)
{
    Q_D(DLocalFile);
    return d->write(byteArray);
}

bool DLocalFile::seek(qint64 pos, DFile::DFMSeekType type)
{
    Q_D(DLocalFile);
    return d->seek(pos, type);
}

qint64 DLocalFile::pos()
{
    Q_D(DLocalFile);
    return d->pos();
}

bool DLocalFile::flush()
{
    Q_D(DLocalFile);
    return d->flush();
}

qint64 DLocalFile::size()
{
    Q_D(DLocalFile);
    return d->size();
}

bool DLocalFile::exists()
{
    Q_D(DLocalFile);
    return d->exists();
}
