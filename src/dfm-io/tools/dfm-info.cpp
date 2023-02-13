// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dfmio_global.h"
#include "dfmio_register.h"

#include "core/diofactory.h"
#include "core/diofactory_p.h"

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-info: %s\n", msg);
}

static void display(const QUrl &url)
{
    QSharedPointer<DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        err_msg("create factory failed.");
        return;
    }

    QSharedPointer<DFileInfo> info = factory->createFileInfo();

    if (!info) {
        err_msg("create file info failed.");
        return;
    }

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
    QUrl url(QString::fromLocal8Bit(uri));

    if (!url.isValid())
        return 1;

    dfmio_init();

    display(url);

    return 0;
}
