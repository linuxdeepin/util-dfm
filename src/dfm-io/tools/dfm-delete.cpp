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
    fprintf(stderr, "dfm-delete: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-delete uri.");
}

static bool delete_file(const QUrl &url)
{
    QSharedPointer<DFMIO::DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        err_msg("create factory failed.");
        return false;
    }

    QSharedPointer<DOperator> op = factory->createOperator();
    if (!op) {
        err_msg("operator create failed.");
        return false;
    }

    bool success = op->deleteFile();
    if (!success) {
        err_msg("delete file failed.");
        return false;
    }

    return true;
}

// delete file with uri.
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

    if (!delete_file(url)) {
        return 1;
    }

    return 0;
}
