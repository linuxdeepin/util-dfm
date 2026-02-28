// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_global.h>
#include <dfm-io/denumerator.h>

#include <QVariant>
#include <QDebug>
#include <QTime>

#include <stdio.h>

USING_IO_NAMESPACE

// show detail
static bool lflag = false;

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-list: %s\n", msg);
}

static void enum_uri(const QUrl &url)
{
    QSharedPointer<DEnumerator> enumerator { new DEnumerator(url) };
    if (!enumerator) {
        err_msg("create enumerator failed.");
        return;
    }

    enumerator->setSortRole(DEnumerator::SortRoleCompareFlag::kSortRoleCompareFileLastRead);
    enumerator->setSortMixed(true);
    enumerator->setSortOrder(Qt::DescendingOrder);

    auto list = enumerator->sortFileInfoList();
    for (auto sortInfo : list) {
        qInfo() << sortInfo->url;
    }
}

void usage()
{
    err_msg("usage: dfm-list [-l] uri.");
}

// list all children in a directory.
int main(int argc, char *argv[])
{
    if (argc != 2 && argc != 3) {
        usage();
        return 1;
    }

    char *uri = nullptr;
    if (argc == 3) {
        if (strcmp(argv[1], "-l") == 0) {
            lflag = true;
            uri = argv[2];
        } else if (strcmp(argv[2], "-l") == 0) {
            lflag = true;
            uri = argv[1];
        } else {
            usage();
            return 1;
        }
    } else {
        // argc == 2
        uri = argv[1];
    }

    QUrl url(QUrl::fromLocalFile(QString::fromLocal8Bit(uri)));

    if (!url.isValid())
        return 1;

    enum_uri(url);

    return 0;
}
