// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dfmio_global.h"
#include "dfmio_register.h"

#include "core/diofactory.h"
#include "core/diofactory_p.h"
#include "core/dfile.h"
#include "local/dlocalfile.h"

#include <QElapsedTimer>
#include <QDebug>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-copy: %s\n", msg);
}

static QSharedPointer<DFile> make_stream(QSharedPointer<DIOFactory> factory, DFile::OpenFlag flag)
{
    if (!factory) {
        err_msg("create factory failed.");
        return nullptr;
    }

    auto file = factory->createFile();
    if (!file) {
        err_msg("get device file failed.");
        return nullptr;
    }
    if (!file->open(flag)) {
        return nullptr;
    }

    return file;
}

static void copy(const QUrl &url_src, const QUrl &url_dst)
{
    const int block = 128 * 1024;
    char buff[block];
    int read = 0;

    QSharedPointer<DFMIO::DIOFactory> factory_src = produceQSharedIOFactory(url_src.scheme(), static_cast<QUrl>(url_src));
    QSharedPointer<DFMIO::DIOFactory> factory_dst = produceQSharedIOFactory(url_dst.scheme(), static_cast<QUrl>(url_dst));

    QSharedPointer<DFile> stream_src = make_stream(factory_src, DFile::OpenFlag::kReadOnly);
    QSharedPointer<DFile> stream_dst = make_stream(factory_dst, DFile::OpenFlag::kWriteOnly);

    if (!stream_src || !stream_dst) {
        return;
    }
    while ((read = stream_src->read(buff, block)) > 0) {
        if (stream_dst->write(buff, read) != read) {
            err_msg("write failed.");
            break;
        }
    }
}

static void usage()
{
    err_msg("usage: dfm-copy src_uri dst_uri.");
}

// copy src to dst with uri names.
int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage();
        return 1;
    }

    const char *uri_src = argv[1];
    const char *uri_dst = argv[2];

    QUrl url_src(QString::fromLocal8Bit(uri_src));
    QUrl url_dst(QString::fromLocal8Bit(uri_dst));

    if (!url_src.isValid() || !url_dst.isValid())
        return 1;

    dfmio_init();

    QElapsedTimer timer;
    timer.start();
    copy(url_src, url_dst);

    auto time = timer.elapsed();
    qInfo() << "dfm-io call elapsed time: (ms)" << time;

    return 0;
}
