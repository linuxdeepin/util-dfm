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
    fprintf(stderr, "dfm-rename: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-rename uri new-name.");
}

static bool rename_file(const QUrl &url, const QString &new_name)
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

    bool success = op->renameFile(new_name);
    if (!success) {
        err_msg("rename file failed.");
        return false;
    }

    return true;
}

// rename uri with new-name.
int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage();
        return 1;
    }

    // do this first.
    dfmio_init();

    const char *uri = argv[1];
    const char *new_name = argv[2];

    QUrl url(QString::fromLocal8Bit(uri));
    if (!url.isValid())
        return 1;

    const QString &qsName = QString::fromLocal8Bit(new_name);

    if (qsName.isEmpty())
        return 1;

    if (!rename_file(url, qsName)) {
        return 1;
    }

    return 0;
}
