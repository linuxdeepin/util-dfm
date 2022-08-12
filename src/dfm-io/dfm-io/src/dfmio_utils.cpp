/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             zhangsheng<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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

#include "dfmio_utils.h"
#include "local/dlocalhelper.h"

#include <gio/gio.h>
#include <gio-unix-2.0/gio/gunixmounts.h>

#include <QUrl>
#include <QSet>

USING_IO_NAMESPACE

bool DFMUtils::fileUnmountable(const QString &path)
{
    g_autoptr(GFile) gfile = g_file_new_for_path(path.toLocal8Bit().data());

    g_autoptr(GMount) gmount = g_file_find_enclosing_mount(gfile, nullptr, nullptr);
    if (gmount) {
        return g_mount_can_unmount(gmount);
    }

    return false;
}

QString DFMUtils::devicePathFromUrl(const QUrl &url)
{
    if (!url.isValid())
        return QString();

    g_autoptr(GFile) gfile = g_file_new_for_uri(url.toString().toLocal8Bit().data());
    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GMount) gmount = g_file_find_enclosing_mount(gfile, nullptr, &gerror);
    if (gmount) {
        g_autoptr(GFile) rootFile = g_mount_get_root(gmount);
        g_autofree gchar *uri = g_file_get_uri(rootFile);
        return QString::fromLocal8Bit(uri);
    } else {
        g_autoptr(GUnixMountEntry) mount = g_unix_mount_for(g_file_peek_path(gfile), nullptr);
        if (mount)
            return QString::fromLocal8Bit(g_unix_mount_get_device_path(mount));
    }
    return QString();
}

QString DFMUtils::fsTypeFromUrl(const QUrl &url)
{
    if (!url.isValid())
        return QString();

    g_autoptr(GFile) gfile = g_file_new_for_uri(url.toString().toLocal8Bit().data());
    g_autofree char *path = g_file_get_path(gfile);
    if (!path)
        return QString();
    g_autoptr(GUnixMountEntry) mount = g_unix_mount_for(path, nullptr);
    if (mount)
        return QString::fromLocal8Bit(g_unix_mount_get_fs_type(mount));
    return QString();
}

QUrl DFMUtils::directParentUrl(const QUrl &url, const bool localFirst /*= true*/)
{
    if (!url.isValid())
        return QUrl();

    auto localPathUrl = [=](GFile *gfile) {
        g_autofree gchar *path = g_file_get_path(gfile);
        if (path)
            return QUrl::fromLocalFile(QString::fromLocal8Bit(path));
        return QUrl();
    };

    g_autoptr(GFile) file = g_file_new_for_uri(url.toString().toLocal8Bit().data());
    g_autoptr(GFile) fileParent = g_file_get_parent(file);
    if (fileParent) {
        if (localFirst) {
            const QUrl urlParent = localPathUrl(fileParent);
            if (urlParent.isValid())
                return urlParent;
        }
        g_autofree gchar *uri = g_file_get_uri(fileParent);
        if (uri)
            return QUrl(QString::fromLocal8Bit(uri));
        return localPathUrl(fileParent);
    }
    return QUrl();
}

bool DFMUtils::fileIsRemovable(const QUrl &url)
{
    if (!url.isValid())
        return false;
    g_autoptr(GFile) gfile = g_file_new_for_uri(url.toString().toLocal8Bit().data());
    g_autoptr(GMount) gmount = g_file_find_enclosing_mount(gfile, nullptr, nullptr);
    if (gmount) {
        g_autoptr(GDrive) gdrive = g_mount_get_drive(gmount);
        if (gdrive)
            return g_drive_is_removable(gdrive);
        else
            return g_mount_can_unmount(gmount);   // when gdrive is nullptr, check unmountable
    }

    return false;
}

QSet<QString> DFMUtils::hideListFromUrl(const QUrl &url)
{
    return DLocalHelper::hideListFromUrl(url);
}
