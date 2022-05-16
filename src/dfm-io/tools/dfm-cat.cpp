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

#include "core/dfile.h"
#include "core/diofactory.h"
#include "core/diofactory_p.h"

#include <gio/gio.h>

#include <QString>
#include <QDebug>

#include <sys/unistd.h>
#include <stdio.h>
#include <iostream>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-cat: %s\n", msg);
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

#define BLOCK (4 * 4096)
static void cat(const QUrl &url)
{
    auto factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));

    if (!factory) {
        err_msg("bad factory.");
        return;
    }

    QSharedPointer<DFile> stream = make_stream(factory, DFile::OpenFlag::kReadOnly);
    if (!stream) {
        err_msg("make stream failed.");
        return;
    }

    char buff[BLOCK];
    int read = 0;
    while ((read = stream->read(buff, BLOCK)) > 0) {
        if (write(STDOUT_FILENO, buff, read) != read) {
            err_msg("write failed.");
            break;
        }
    }
}

static void usage()
{
    err_msg("usage: dfm-cat uri.");
}

// display file content.
int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        return 1;
    }

    const char* uri = argv[1];
    QUrl url(QString::fromLocal8Bit(uri));

    if (!url.isValid())
        return 1;

    dfmio_init();
    //REGISTER_FACTORY1(DLocalIOFactory, url.scheme(), QUrl);

    cat(url);

    return 0;
}
