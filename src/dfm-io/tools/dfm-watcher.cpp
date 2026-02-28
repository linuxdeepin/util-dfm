// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dwatcher.h>

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

static bool watcher_dir(const QUrl &url, QSharedPointer<DWatcher> &watcher)
{
    watcher.reset(new DWatcher(url));
    if (!watcher) {
        err_msg("watcher create failed.");
        return false;
    }

    QObject::connect(watcher.get(), &DWatcher::fileChanged, [](const QUrl &url) {
        qDebug() << "file changed: " << url;
        printf("file changed:%s.\n", url.url().toLocal8Bit().data());
    });

    QObject::connect(watcher.get(), &DWatcher::fileDeleted, [](const QUrl &url) {
        qDebug() << "file deleted: " << url;
        printf("file deleted:%s.\n", url.url().toLocal8Bit().data());
    });

    QObject::connect(watcher.get(), &DWatcher::fileAdded, [](const QUrl &url) {
        qDebug() << "file added: " << url;
        printf("file added:%s.\n", url.url().toLocal8Bit().data());
    });
    QObject::connect(watcher.get(), &DWatcher::fileRenamed, [](const QUrl &fromUri, const QUrl &toUri) {
        qDebug() << "file move: " << fromUri << "to" << toUri;
        printf("file move: %s to %s.\n", fromUri.url().toLocal8Bit().data(), toUri.url().toLocal8Bit().data());
    });

    if (!watcher->start()) {
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

    QCoreApplication app(argc, argv);

    const char *uri = argv[1];
    QUrl url(QUrl::fromLocalFile(QString::fromLocal8Bit(uri)));
    if (!url.isValid())
        return 1;

    QSharedPointer<DWatcher> watcher = nullptr;
    if (!watcher_dir(url, watcher)) {
        return 1;
    }

    return app.exec();
}
