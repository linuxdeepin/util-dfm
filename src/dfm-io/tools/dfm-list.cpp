// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_global.h>
#include <dfm-io/denumerator.h>
#include <dfm-io/denumeratorfuture.h>

#include <stdio.h>

#include <QDebug>
#include <QUrl>
#include <QCoreApplication>
#include <QThread>
#include <QtConcurrent>

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

    int count { 0 };
    while (enumerator->hasNext()) {
        const QUrl &url = enumerator->next();
        qInfo() << url;
        ++count;
    }
    qInfo() << "count: " << count;
    enumerator.clear();
}

static void enum_uri_async_test(const QUrl &url)
{
    static QSharedPointer<DEnumerator> enumerator { new DEnumerator(url) };
    if (!enumerator) {
        err_msg("create enumerator failed.");
        return;
    }
    auto future = enumerator->asyncIterator();
    QObject::connect(future, &DEnumeratorFuture::asyncIteratorOver, [=]() {
        int count { 0 };
        while (enumerator->hasNext()) {
            const QUrl &url = enumerator->next();
            qInfo() << url;
            ++count;
        }
        qInfo() << "count: " << count;
        enumerator.clear();
    });
    future->startAsyncIterator();
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

    QCoreApplication a(argc, argv);

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

    enum_uri_async_test(url);

    return a.exec();
}
