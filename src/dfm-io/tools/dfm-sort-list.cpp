// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dfmio_global.h"
#include "dfmio_register.h"

#include "core/diofactory.h"
#include "core/diofactory_p.h"

#include <QVariant>
#include <QDebug>

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
    QSharedPointer<DIOFactory> factory = produceQSharedIOFactory(url.scheme(), static_cast<QUrl>(url));
    if (!factory) {
        err_msg("create factory failed.");
        return;
    }

    QSharedPointer<DEnumerator> enumerator = factory->createEnumerator();
    if (!enumerator) {
        err_msg("create enumerator failed.");
        return;
    }

    QMap<DEnumerator::ArgumentKey, QVariant> argus;
    argus.insert(DEnumerator::ArgumentKey::kArgumentSortRole,
                 QVariant::fromValue(DFMIO::DEnumerator::SortRoleCompareFlag::kSortRoleCompareFileLastModified));
    argus.insert(DEnumerator::ArgumentKey::kArgumentMixDirAndFile, true);
    argus.insert(DEnumerator::ArgumentKey::kArgumentSortOrder, Qt::DescendingOrder);
    enumerator->setArguments(argus);
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

    QUrl url(QString::fromLocal8Bit(uri));

    if (!url.isValid())
        return 1;

    dfmio_init();

    enum_uri(url);

    return 0;
}
