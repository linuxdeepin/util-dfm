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
    if (!gfile)
        return false;

    GOutputStream *output_stream = nullptr;
    GInputStream *input_stream = nullptr;

    GError *error = nullptr;
    if (mode == DFile::OpenFlag::ReadOnly) {
        input_stream = (GInputStream*)g_file_read(gfile, nullptr, &error);
        if (error)
            qWarning() << error->message;

        if (!input_stream) {
            g_object_unref(gfile);
            gfile = nullptr;
            return false;
        }
    } else if (mode == DFile::OpenFlag::WriteOnly) {
        output_stream = (GOutputStream *)g_file_replace(gfile,
                                                        nullptr,
                                                        false,
                                                        G_FILE_CREATE_NONE,
                                                        nullptr,
                                                        &error);
        if (error)
            qWarning() << error->message;

        if (!output_stream) {
            g_object_unref(gfile);
            gfile = nullptr;
            return false;
        }
    }

    this->input_stream = input_stream;
    this->output_stream = output_stream;

    if (gfile)
        g_object_unref(gfile);

    return true;
}

bool DLocalFilePrivate::close()
{
    if (input_stream) {
        g_input_stream_close(input_stream, nullptr, nullptr);
        g_object_unref(input_stream);
        input_stream = nullptr;
    }

    if (output_stream) {
        g_output_stream_close(output_stream, nullptr, nullptr);
        g_object_unref(output_stream);
        output_stream = nullptr;
    }
    return true;
}

qint64 DLocalFilePrivate::read(char *data, qint64 maxSize)
{
    if (!input_stream) {
        qWarning() << "need input_stream before read.";
        return -1;
    }

    GError *error = nullptr;
    gssize read = g_input_stream_read(input_stream,
                                      data,
                                      maxSize,
                                      nullptr,
                                      &error);

    if (read < 0) {
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
            return -1;
        }
    }

    return read;
}

QByteArray DLocalFilePrivate::read(qint64 maxSize)
{
    if (!input_stream) {
        qWarning() << "need input_stream before read.";
        return QByteArray();
    }

    char data[maxSize];
    GError *error = nullptr;
    gssize read = g_input_stream_read(input_stream,
                                      data,
                                      maxSize,
                                      nullptr,
                                      &error);

    if (read < 0) {
        if (error) {
            qWarning() << error->message;
            g_error_free(error);
            return QByteArray();
        }
    }

    return QByteArray(data);
}

QByteArray DLocalFilePrivate::readAll()
{
    if (!input_stream) {
        qWarning() << "need input_stream before read.";
        return QByteArray();
    }
    QByteArray dataRet;

    const uint16_t &size = 2048;

    gsize bytes_read;
    char data[size];
    GError *error = nullptr;

    while (true) {
        gboolean read = g_input_stream_read_all(input_stream,
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
    if (output_stream == nullptr) {
        qWarning() << "need output_stream before write.";
        return -1;
    }

    GError *error = nullptr;
    gssize write = g_output_stream_write(output_stream,
                                         data,
                                         maxSize,
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
    if (output_stream == nullptr) {
        qWarning() << "need output_stream before write.";
        return -1;
    }

    gsize bytes_write;
    GError *error = nullptr;
    gssize write = g_output_stream_write_all(output_stream,
                                         data,
                                         G_MAXSSIZE,
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

bool DLocalFilePrivate::seek(qint64 pos)
{
    Q_UNUSED(pos);

    return false;
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
    registerSeek(std::bind(&DLocalFile::seek, this, std::placeholders::_1));
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

bool DLocalFile::seek(qint64 pos)
{
    Q_D(DLocalFile);
    return d->seek(pos);
}
