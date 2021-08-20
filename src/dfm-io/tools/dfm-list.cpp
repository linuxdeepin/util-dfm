/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dfmio_global.h"
#include "dfmio_register.h"

#include "core/diofactory.h"
#include "core/diofactory_p.h"

#include <stdio.h>

#include <QVariant>

USING_IO_NAMESPACE

// show detail
static bool lflag = false;

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-list: %s\n", msg);
}

static void show_fileinfo(QSharedPointer<DFileInfo> info)
{
    if (!info)
        return;

    if (lflag) {
        printf("%s\n", info->dump().toStdString().c_str());
    } else {
        // only show display-name
        bool success = false;
        const QString &path = info->attribute(DFileInfo::AttributeID::StandardDisplayName, success).toString();
        printf("%s", path.toStdString().c_str());
    }
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

    while (enumerator->hasNext()) {
        enumerator->next();

        show_fileinfo(enumerator->fileInfo());
        printf("\n");
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
    //REGISTER_FACTORY1(DLocalIOFactory, url.scheme(), QUrl);

    enum_uri(url);

    return 0;
}
