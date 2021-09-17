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

#include "dfmio_global.h"
#include "dfmio_register.h"

#include "core/diofactory.h"
#include "core/diofactory_p.h"
#include "core/dfile.h"

#include <gio/gio.h>

#include <QElapsedTimer>
#include <QDebug>

#include <stdio.h>

USING_IO_NAMESPACE

qint64 readData(GFileInputStream *input_stream, char *data, qint64 maxlen)
{
    GError *error = nullptr;

    qint64 size = g_input_stream_read((GInputStream*)input_stream, data, static_cast<gsize>(maxlen), nullptr, &error);

    if (error) {
        g_error_free(error);
        return -1;
    }
    return size;
}

qint64 writeData(GFileOutputStream *output_stream, const char *data, qint64 len)
{
    GError *error = nullptr;

    qint64 size = g_output_stream_write((GOutputStream*)output_stream, data, static_cast<gsize>(len), nullptr, &error);
    if (error) {
        g_error_free(error);
        return -1;
    }
    return size;
}

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-copy: %s\n", msg);
}

static void copy(const QString &url_src, const QString &url_dst)
{
    const int block = 128 * 1024;
    char buff[block];
    int read = 0;

    GError *error = nullptr;
    GFile *gfile = g_file_new_for_path(url_src.toLocal8Bit().data());
    GFileInputStream *inputStream = g_file_read(gfile, nullptr, &error);

    GFile *gfileOut = g_file_new_for_path(url_dst.toLocal8Bit().data());
    GFileOutputStream *outputStream = g_file_replace(gfileOut,
                                                     nullptr,
                                                     false,
                                                     G_FILE_CREATE_NONE,
                                                     nullptr,
                                                     &error);
    while ((read = readData(inputStream, buff, block)) > 0) {
        if (writeData(outputStream, buff, read) != read) {
            err_msg("write failed.");
            break;
        }
    }

    g_input_stream_close((GInputStream*)inputStream, nullptr, nullptr);
    g_output_stream_close((GOutputStream*)outputStream, nullptr, nullptr);
    g_object_unref(inputStream);
    g_object_unref(outputStream);
}

static void usage()
{
    err_msg("usage: dfm-copy src_uri dst_uri.");
}

// copy src to dst with uri names.
int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage();
        return 1;
    }

    const char *uri_src = argv[1];
    const char *uri_dst = argv[2];

    dfmio_init();

    QElapsedTimer timer;
    timer.start();
    copy(uri_src, uri_dst);

    auto time = timer.elapsed();
    qInfo() << "gio func direct call elapsed time: (ms)" << time;

    return 0;
}
