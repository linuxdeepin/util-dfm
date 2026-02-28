// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/doperator.h>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-link: %s\n", msg);
}

static void usage()
{
    err_msg("usage: dfm-link link src-path.");
}

static bool link_file(const QUrl &url, const QUrl &urlto)
{
    QSharedPointer<DOperator> op { new DOperator(url) };
    if (!op) {
        err_msg("operator create failed.");
        return false;
    }

    bool success = op->createLink(urlto);
    if (!success) {
        err_msg("create file link failed.");
        return false;
    }

    return true;
}

// link uri to dst-uri.
int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage();
        return 1;
    }

    const char *uri = argv[1];
    const char *urito = argv[2];

    QUrl urlSource(QUrl::fromLocalFile(QString::fromLocal8Bit(uri)));
    QUrl urlTarget(QUrl::fromLocalFile(QString::fromLocal8Bit(urito)));

    if (!urlSource.isValid())
        return -1;

    if (!link_file(urlSource, urlTarget)) {
        return 1;
    }

    return 0;
}
