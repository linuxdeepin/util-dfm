// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_global.h>
#include <dfm-io/denumerator.h>
#include <dfm-io/denumeratorfuture.h>
#include <dfm-io/dfileinfo.h>

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

static void initEnumerator_demo(const QUrl &url)
{
    qInfo() << "--- initEnumerator Demo ---";

    QSharedPointer<DEnumerator> enumerator { new DEnumerator(url) };
    if (!enumerator) {
        err_msg("create enumerator failed.");
        return;
    }

    // Demo 1: oneByone = true (GIO mode)
    qInfo() << "Demo 1: oneByone = true (GIO mode)";
    bool result1 = enumerator->initEnumerator(true);
    qInfo() << "initEnumerator(true) result:" << result1;
    if (result1) {
        int count = 0;
        while (enumerator->hasNext()) {
            const QUrl &nextUrl = enumerator->next();
            QSharedPointer<DFileInfo> fileInfo = enumerator->fileInfo();
            if (fileInfo) {
                QString fileName = fileInfo->attribute(DFileInfo::AttributeID::kStandardName).toString();
                bool isDir = fileInfo->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool();
                qint64 size = fileInfo->attribute(DFileInfo::AttributeID::kStandardSize).toLongLong();
                qInfo() << "File:" << fileName << "Dir:" << isDir << "Size:" << size;
            }
            ++count;
        }
        qInfo() << "GIO mode count:" << count;
    } else {
        DFMIOError error = enumerator->lastError();
        qInfo() << "GIO mode failed:" << error.errorMsg();
    }

    // Demo 2: oneByone = false (FTS mode)
    qInfo() << "Demo 2: oneByone = false (FTS mode)";
    QSharedPointer<DEnumerator> enumerator2 { new DEnumerator(url) };
    if (enumerator2) {
        bool result2 = enumerator2->initEnumerator(false);
        qInfo() << "initEnumerator(false) result:" << result2;
        if (result2) {
            // Use sortFileInfoList for FTS mode
            QList<QSharedPointer<DEnumerator::SortFileInfo>> sortList = enumerator2->sortFileInfoList();
            qInfo() << "FTS mode count:" << sortList.size();
            for (const auto &sortInfo : sortList) {
                QString fileName = sortInfo->url.fileName();
                qInfo() << "File:" << fileName << "Dir:" << sortInfo->isDir << "Size:" << sortInfo->filesize;
            }
        } else {
            DFMIOError error = enumerator2->lastError();
            qInfo() << "FTS mode failed:" << error.errorMsg();
        }
    }

    // Demo 3: With filters
    qInfo() << "Demo 3: With filters";
    QSharedPointer<DEnumerator> enumerator3 { new DEnumerator(url, QStringList(),
                                                             DEnumerator::DirFilters(DEnumerator::DirFilter::kFiles),
                                                             DEnumerator::IteratorFlags()) };
    if (enumerator3) {
        bool result3 = enumerator3->initEnumerator(true);
        qInfo() << "initEnumerator with filters result:" << result3;
        if (result3) {
            int count = 0;
            while (enumerator3->hasNext()) {
                const QUrl &nextUrl = enumerator3->next();
                QSharedPointer<DFileInfo> fileInfo = enumerator3->fileInfo();
                if (fileInfo) {
                    QString fileName = fileInfo->attribute(DFileInfo::AttributeID::kStandardName).toString();
                    bool isFile = fileInfo->attribute(DFileInfo::AttributeID::kStandardIsFile).toBool();
                    qInfo() << "File only:" << fileName << "IsFile:" << isFile;
                }
                ++count;
            }
            qInfo() << "Filtered files count:" << count;
        } else {
            DFMIOError error = enumerator3->lastError();
            qInfo() << "Filtered mode failed:" << error.errorMsg();
        }
    }

    qInfo() << "--- initEnumerator Demo End ---";
}

void usage()
{
    err_msg("usage: dfm-list [-l] uri.");
    err_msg("usage: dfm-list [-d] uri.  # run initEnumerator demo");
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
    bool demoMode = false;

    if (argc == 3) {
        if (strcmp(argv[1], "-l") == 0) {
            lflag = true;
            uri = argv[2];
        } else if (strcmp(argv[2], "-l") == 0) {
            lflag = true;
            uri = argv[1];
        } else if (strcmp(argv[1], "-d") == 0) {
            demoMode = true;
            uri = argv[2];
        } else if (strcmp(argv[2], "-d") == 0) {
            demoMode = true;
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

    if (demoMode) {
        initEnumerator_demo(url);
        return 0;
    } else {
        enum_uri(url);
        enum_uri_async_test(url);
    }

    return a.exec();
}
