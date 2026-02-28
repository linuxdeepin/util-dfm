// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/doperator.h>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-restore: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-restore trash-file-uri.");
}

static void restore(const QUrl &url)
{
    QSharedPointer<DOperator> op { new DOperator(url) };
    if (!op) {
        err_msg("operator create failed.");
        return;
    }

    if (!op->restoreFile()) {
        err_msg("restore file failed.");
        return;
    }
}

// restore file from trash.
int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        return 1;
    }

    const char *uri = argv[1];
    QUrl url(QUrl::fromLocalFile(QString::fromLocal8Bit(uri)));

    restore(url);

    return 0;
}
