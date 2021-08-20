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

#include <QObject>
#include <QDebug>
#include <QCoreApplication>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-watcher: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-watcher uri.");
}

static bool watcher_dir(const QUrl &url)
{
    QSharedPointer<DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        err_msg("create factory failed.");
        return false;
    }

    QSharedPointer<DWatcher> watcher = factory->createWatcher();
    if (!watcher) {
        err_msg("watcher create failed.");
        return false;
    }

    QObject::connect(watcher.get(), &DWatcher::fileChanged, [](const QUrl &uri, const DFileInfo &fileInfo) {
        Q_UNUSED(fileInfo);
        printf("file changed:%s.\n", uri.url().toStdString().c_str());
    });

    QObject::connect(watcher.get(), &DWatcher::fileDeleted, [](const QUrl &uri, const DFileInfo &fileInfo) {
        Q_UNUSED(fileInfo);
        printf("file deleted:%s.\n", uri.url().toStdString().c_str());
    });

    QObject::connect(watcher.get(), &DWatcher::fileAdded, [](const QUrl &uri, const DFileInfo &fileInfo) {
        Q_UNUSED(fileInfo);
        printf("file added:%s.\n", uri.url().toStdString().c_str());
    });

    if (!watcher->start(0)) {
        err_msg("start watcher failed.");
        return false;
    }

    return true;
}

// watch directory.
int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        return 1;
    }

    // do this first.
    dfmio_init();

    QCoreApplication app(argc, argv);

    const char *uri = argv[1];
    QUrl url(QString::fromLocal8Bit(uri));
    if (!url.isValid())
        return 1;

    if (!watcher_dir(url)) {
        return 1;
    }

    return app.exec();
}
