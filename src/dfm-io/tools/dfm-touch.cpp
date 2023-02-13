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
    fprintf(stderr, "dfm-touch: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-touch uri.");
}

static bool touch_file(const QUrl &url)
{
    QSharedPointer<DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        err_msg("create factory failed.");
        return false;
    }

    QSharedPointer<DOperator> op = factory->createOperator();
    if (!op) {
        err_msg("operator create failed.");
        return false;
    }

    bool success = op->touchFile();
    if (!success) {
        err_msg("touch file failed.");
        return false;
    }

    return true;
}

// ensure file with uri is exists.
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

    if (!touch_file(url)) {
        return 1;
    }
    
    return 0;
}
