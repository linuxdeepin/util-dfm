// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/doperator.h>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-move: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-move uri new-path.");
}

static bool move_file(const QUrl &url_src, const QUrl &url_dst)
{
    QSharedPointer<DOperator> op { new DOperator(url_src) };
    if (!op) {
        err_msg("operator create failed.");
        return false;
    }

    bool success = op->moveFile(url_dst, DFile::CopyFlag::kNone);
    if (!success) {
        err_msg("move file failed.");
        return false;
    }

    return true;
}

// move uri to new-path.
int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage();
        return 1;
    }

    const char *uri_src = argv[1];
    const char *uri_dst = argv[2];

    QUrl url_src(QUrl::fromLocalFile(QString::fromLocal8Bit(uri_src)));
    QUrl url_dst(QUrl::fromLocalFile(QString::fromLocal8Bit(uri_dst)));

    if (!url_src.isValid() || !url_dst.isValid())
        return 1;

    if (!move_file(url_src, url_dst)) {
        return 1;
    }

    return 0;
}
