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
#include "core/dfile.h"
#include "local/dlocalhelper.h"

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
    GFile *gfileSource = g_file_new_for_uri(sourcePath.toLocal8Bit().data());
    GFile *gfileDest = g_file_new_for_uri(destPath.toLocal8Bit().data());

    GFile *gfileTarget = nullptr;
    if (DLocalHelper::checkGFileType(gfileDest, G_FILE_TYPE_DIRECTORY)) {
        char *basename = g_file_get_basename (gfileSource);
        gfileTarget = g_file_get_child(gfileDest, basename);
        g_free(basename);
    } else {
        gfileTarget = g_file_new_for_uri(destPath.toLocal8Bit().data());
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
