// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfile.h>

#include <QElapsedTimer>
#include <QDebug>

#include <stdio.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-copy: %s\n", msg);
}

static QSharedPointer<DFile> make_stream(const QUrl &url, DFile::OpenFlag flag)
{
    QSharedPointer<DFile> file { new DFile(url) };
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
    qint64 read = 0;

    QSharedPointer<DFile> stream_src = make_stream(url_src, DFile::OpenFlag::kReadOnly);
    QSharedPointer<DFile> stream_dst = make_stream(url_dst, DFile::OpenFlag::kWriteOnly);

    if (!stream_src || !stream_dst) {
        return;
    }

    while ((read = stream_src->read(buff, block)) > 0) {
        qint64 sizeall = 0, sizeWrite = 0, sizeRead = read;
        char *surplusData = buff;
        do {
            sizeWrite = stream_dst->write(surplusData, sizeRead);
            sizeall += sizeWrite;
            surplusData += sizeWrite;
            sizeRead -= sizeWrite;
            if (sizeWrite < 0 || (sizeWrite == 0 && sizeRead > 0)){
                err_msg("write failed.");
                return;
            }
        } while (sizeall < read);
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

    QUrl url_src(QUrl::fromLocalFile(QString::fromLocal8Bit(uri_src)));
    QUrl url_dst(QUrl::fromLocalFile(QString::fromLocal8Bit(uri_dst)));

    if (!url_src.isValid() || !url_dst.isValid())
        return 1;

    QElapsedTimer timer;
    timer.start();
    copy(url_src, url_dst);

    auto time = timer.elapsed();
    qInfo() << "dfm-io call elapsed time: (ms)" << time;

    return 0;
}
