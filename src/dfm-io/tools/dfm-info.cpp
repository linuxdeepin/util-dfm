// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfileinfo.h>

#include <QDebug>
#include <QTime>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-info: %s\n", msg);
}

static void display(const QUrl &url)
{
    QSharedPointer<DFileInfo> info { new DFileInfo(url) };

    if (!info) {
        err_msg("create file info failed.");
        return;
    }

    printf("%s\n", info->dump().toStdString().c_str());
}

static void displayAsync(const QUrl &url)
{
    QSharedPointer<DFileInfo> info { new DFileInfo(url) };

    if (!info) {
        err_msg("create file info failed.");
        return;
    }
    auto future = info->refreshAsync();
    future.waitForFinished();
    printf("%s\n", info->dump().toStdString().c_str());
}

static void usage()
{
    err_msg("usage: dfm-info uri.");
}

// get file info with uri.
int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        return 1;
    }

    const char *uri = argv[1];
    QUrl url(QUrl::fromLocalFile(QString::fromLocal8Bit(uri)));

    if (!url.isValid())
        return 1;
    display(url);
    displayAsync(url);

    return 0;
}
