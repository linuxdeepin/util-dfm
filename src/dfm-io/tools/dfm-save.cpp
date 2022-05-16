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

#include <stdio.h>
#include <sys/unistd.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-save: %s\n", msg);
}

static QSharedPointer<DFile> make_stream(QSharedPointer<DIOFactory> factory, DFile::OpenFlag flag)
{
    if (!factory) {
        err_msg("create factory failed.");
        return nullptr;
    }

    auto file = factory->createFile();
    if (!file) {
        err_msg("get device file failed.");
        return nullptr;
    }
    if (!file->open(flag)) {
        return nullptr;
    }

    return file;
}

#define BLOCK 4 * 4096
static void save(const QUrl &url)
{
    QSharedPointer<DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));

    QSharedPointer<DFile> stream = make_stream(factory, DFile::OpenFlag::kWriteOnly);
    if (!stream) {
        err_msg("make stream failed.");
        return;
    }

    char buff[BLOCK];
    int bytes = 0;
    while ((bytes = read(STDIN_FILENO, buff, BLOCK)) > 0) {
        if (stream->write(buff, bytes) != bytes)
            err_msg("write failed.");
    }
}

static void usage()
{
    err_msg("usage: dfm-save uri.");
}

// copy src to dst with uri names.
int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        return 1;
    }

    // do this first.
    dfmio_init();

    const char *uri = argv[1];

    QUrl url(QString::fromLocal8Bit(uri));

    if (!url.isValid())
        return 1;

    save(url);

    return 0;
}
