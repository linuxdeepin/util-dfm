// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../dfm-io/utils/dlocalhelper.h"   // for test

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfile.h>

#include <gio/gio.h>

#include <QElapsedTimer>
#include <QDebug>

#include <stdio.h>
#include <fcntl.h>

USING_IO_NAMESPACE

static void err_msg(const char *msg)
{
    fprintf(stderr, "dfm-copy: %s\n", msg);
}

static void copy(const QString &sourcePath, const QString &destPath)
{
    GFile *gfileSource = DLocalHelper::createGFile(sourcePath);
    GFile *gfileDest = DLocalHelper::createGFile(destPath);

    GFile *gfileTarget = nullptr;
    if (DLocalHelper::checkGFileType(gfileDest, G_FILE_TYPE_DIRECTORY)) {
        char *basename = g_file_get_basename(gfileSource);
        gfileTarget = g_file_get_child(gfileDest, basename);
        g_free(basename);
    } else {
        gfileTarget = DLocalHelper::createGFile(destPath);
    }

    //预先读取
    {
        char *gpath = g_file_get_path(gfileSource);
        int fromfd = ::open(gpath, O_RDONLY);
        if (-1 != fromfd) {
            GError *gerror = nullptr;
            GFileInfo *gfileinfo = g_file_query_info(gfileSource, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, nullptr, &gerror);
            if (gfileinfo) {
                goffset size = g_file_info_get_size(gfileinfo);
                readahead(fromfd, 0, static_cast<size_t>(size));
                g_object_unref(gfileinfo);
            }
            if (gerror)
                g_error_free(gerror);
            close(fromfd);
        }
        g_free(gpath);
    }

    GError *gerror = nullptr;

    g_file_copy(gfileSource, gfileTarget, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, &gerror);

    g_object_unref(gfileSource);
    g_object_unref(gfileDest);
    g_object_unref(gfileTarget);

    if (gerror)
        g_error_free(gerror);
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

    QElapsedTimer timer;
    timer.start();
    copy(uri_src, uri_dst);

    auto time = timer.elapsed();
    qInfo() << "gio g_file_copy call elapsed time: (ms)" << time;

    return 0;
}
