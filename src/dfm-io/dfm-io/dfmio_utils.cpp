// SPDX-FileCopyrightText: 2021 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_utils.h>

#include "utils/dlocalhelper.h"

#include <gio/gio.h>
#include <gio-unix-2.0/gio/gunixmounts.h>

#include <QUrl>
#include <QSet>
#include <QDebug>

USING_IO_NAMESPACE

bool DFMUtils::fileUnmountable(const QString &path)
{
    if (path.isEmpty())
        return false;

    g_autoptr(GFile) gfile = g_file_new_for_path(path.toStdString().c_str());
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

    g_autoptr(GFile) gfile = g_file_new_for_uri(url.toString().toStdString().c_str());
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

QString DFMUtils::buildFilePath(const char *segment, ...)
{
    va_list args;
    va_start(args, segment);
    g_autofree gchar *str = g_build_filename_valist(segment, &args);
    va_end(args);

    return QString::fromLocal8Bit(str);
}

QStringList DFMUtils::systemDataDirs()
{
    QStringList lst;
    const char *const *cresult = g_get_system_data_dirs();
    if (!cresult)
        return {};

    for (const gchar *const *iter = cresult; *iter != nullptr; ++iter) {
        lst.append(QString::fromLocal8Bit(*iter));
    }

    return lst;
}

QString DFMUtils::userSpecialDir(DGlibUserDirectory userDirectory)
{
    const gchar *str = g_get_user_special_dir(static_cast<GUserDirectory>(userDirectory));
    return QString::fromLocal8Bit(str);
}

QString DFMUtils::userDataDir()
{
    const gchar *dir = g_get_user_data_dir();
    return QString::fromLocal8Bit(dir);
}
