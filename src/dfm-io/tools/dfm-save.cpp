// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfile.h>

#include <stdio.h>
#include <sys/unistd.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-save: %s\n", msg);
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

#define BLOCK 4 * 4096
static void save(const QUrl &url)
{

    QSharedPointer<DFile> stream = make_stream(url, DFile::OpenFlag::kWriteOnly);
    if (!stream) {
        err_msg("make stream failed.");
        return;
    }

    char buff[BLOCK];
    int bytes = 0;
    while ((bytes = read(STDIN_FILENO, buff, BLOCK)) > 0) {
        if (stream->write(buff, bytes) != bytes)
            err_msg("write failed.");
    }
}

static void usage()
{
    err_msg("usage: dfm-save uri.");
}

// copy src to dst with uri names.
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

    save(url);

    return 0;
}
