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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

USING_IO_NAMESPACE

qint64 readData(int m_fileFd, char *data, qint64 maxlen)
{
    return ::read(m_fileFd, data, static_cast<size_t>(maxlen));
}

qint64 writeData(int m_fileFd, const char *data, qint64 maxlen)
{
    return ::write(m_fileFd, data, static_cast<size_t>(maxlen));
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

    int m_fileFd = ::open(url_src.toLocal8Bit().data(), O_RDONLY, 0755);
    int m_fileFd2 = ::open(url_dst.toLocal8Bit().data(), O_CREAT | O_WRONLY, 0755);

    while ((read = readData(m_fileFd, buff, block)) > 0) {
        if (writeData(m_fileFd2, buff, read) != read) {
            err_msg("write failed.");
            break;
        }
    }
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
    qInfo() << "system call elapsed time: (ms)" << time;

    return 0;
}
