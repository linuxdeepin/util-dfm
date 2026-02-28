// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/doperator.h>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-trash: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-trash uri.");
}

static bool trash_file(const QUrl &url)
{
    QSharedPointer<DOperator> op { new DOperator(url) };
    if (!op) {
        err_msg("operator create failed.");
        return false;
    }

    QString targetTrash = op->trashFile();
    if (targetTrash.isEmpty()) {
        err_msg("trash file failed.");
        return false;
    }

    return true;
}

// move file to trash.
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

    if (!trash_file(url)) {
        return 1;
    }

    return 0;
}
