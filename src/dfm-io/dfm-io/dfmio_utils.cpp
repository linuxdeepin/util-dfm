// SPDX-FileCopyrightText: 2021 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dfmio_utils.h>
#include <dfm-io/denumeratorfuture.h>

#include "utils/dlocalhelper.h"

#include <gio/gio.h>
#include <gio-unix-2.0/gio/gunixmounts.h>
#include <glib/gstdio.h>

#include <QUrl>
#include <QSet>
#include <QDebug>
#include <QByteArray>

#include <fstab.h>
#include <sys/stat.h>

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

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(url);
    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GMount) gmount = g_file_find_enclosing_mount(gfile, nullptr, &gerror);
    if (gmount) {
        g_autoptr(GFile) rootFile = g_mount_get_root(gmount);
        g_autofree gchar *uri = g_file_get_uri(rootFile);
        return QString::fromLocal8Bit(uri);
    } else {
        g_autofree gchar *path = g_file_get_path(gfile);
        g_autoptr(GUnixMountEntry) mount = g_unix_mount_for(path, nullptr);
        if (mount) {
            const gchar *devicePath = g_unix_mount_get_device_path(mount);
            return QString::fromLocal8Bit(devicePath);
        }
    }
    return QString();
}

QString DFMUtils::deviceNameFromUrl(const QUrl &url)
{
    if (!url.isValid())
        return QString();

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(url);
    g_autoptr(GUnixMountEntry) mount = g_unix_mount_for(g_file_peek_path(gfile), nullptr);
    if (mount)
        return QString::fromLocal8Bit(g_unix_mount_get_device_path(mount));
    return QString();
}

QString DFMUtils::fsTypeFromUrl(const QUrl &url)
{
    if (!url.isValid())
        return QString();

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(url);
    g_autofree char *path = g_file_get_path(gfile);
    if (!path)
        return QString();
    g_autoptr(GUnixMountEntry) mount = g_unix_mount_for(path, nullptr);
    if (mount)
        return QString::fromLocal8Bit(g_unix_mount_get_fs_type(mount));
    return QString();
}

QString DFMUtils::mountPathFromUrl(const QUrl &url)
{
    if (!url.isValid())
        return QString();

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(url);
    g_autofree char *path = g_file_get_path(gfile);
    if (!path)
        return QString();
    g_autoptr(GUnixMountEntry) mount = g_unix_mount_for(path, nullptr);
    if (mount)
        return QString::fromLocal8Bit(g_unix_mount_get_mount_path(mount));
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

    g_autoptr(GFile) file = DLocalHelper::createGFile(url);
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
    g_autoptr(GFile) gfile = DLocalHelper::createGFile(url);
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

QString DFMUtils::bindPathTransform(const QString &path, bool toDevice)
{
    if (!path.startsWith("/") || path == "/")
        return path;

    const QMap<QString, QString> &table = fstabBindInfo();
    if (table.isEmpty())
        return path;

    QString bindPath(path);
    if (toDevice) {
        for (const auto &mntPoint : table.values()) {
            if (path.startsWith(mntPoint)) {
                bindPath.replace(mntPoint, table.key(mntPoint));
                break;
            }
        }
    } else {
        for (const auto &device : table.keys()) {
            if (path.startsWith(device)) {
                bindPath.replace(device, table[device]);
                break;
            }
        }
    }

    return bindPath;
}

int DFMUtils::dirFfileCount(const QUrl &url)
{
    if (!url.isValid())
        return 0;
    DFMIO::DEnumerator enumerator(url);
    return int(enumerator.fileCount());
}

QUrl DFMUtils::bindUrlTransform(const QUrl &url)
{
    auto tmp = url;

    if (!url.path().contains("\\")) {
        tmp.setPath(bindPathTransform(url.path(), false));
        return tmp;
    }

    auto path = BackslashPathToNormal(url.path());
    path = bindPathTransform(path, false);
    path = normalPathToBackslash(path);
    tmp.setPath(path);
    return tmp;
}

QString DFMUtils::BackslashPathToNormal(const QString &trash)
{
    if (!trash.contains("\\"))
        return trash;
    QString normal = trash;
    normal = normal.replace("\\", "/");
    normal = normal.replace("//", "/");
    return normal;
}

QString DFMUtils::normalPathToBackslash(const QString &normal)
{
    QString trash = normal;
    trash = trash.replace("/", "\\");
    trash.push_front("/");
    return trash;
}

DEnumeratorFuture *DFMUtils::asyncTrashCount()
{
    QSharedPointer<DEnumerator> enumerator(new DEnumerator(QUrl("trash:///")));
    return enumerator->asyncIterator();
}

int DFMUtils::syncTrashCount()
{
    DEnumerator enumerator(QUrl("trash:///"));
    QList<QUrl> children;
    while (enumerator.hasNext()) {
        auto url = DFMUtils::bindUrlTransform(enumerator.next());
        if (!children.contains(url))
            children.append(url);
    }

    return children.length();
}

// 传入的url不能是链接文件，如果是链接文件就是链接文件所在磁盘的数据
qint64 DFMUtils::deviceBytesFree(const QUrl &url)
{
    if (!url.isValid())
        return 0;
    auto path = url.path();
    g_autoptr(GFile) file = g_file_new_for_path(QFile::encodeName(path).constData());
    GError *error = nullptr;
    g_autoptr(GFileInfo)  gioInfo = g_file_query_filesystem_info(file, "filesystem::*", nullptr, &error);

    if (error || !gioInfo) {
        if (error)
            g_error_free(error);
        error = nullptr;
        return std::numeric_limits<qint64>::max();
    }

    quint64 bytesTotal = 0;

    if (g_file_info_has_attribute(gioInfo, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE)) {
        bytesTotal = g_file_info_get_attribute_uint64(gioInfo, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
    } else {
        qInfo() << "file do not support G_FILE_ATTRIBUTE_FILESYSTEM_SIZE, returns max of qint64";
        return std::numeric_limits<qint64>::max();
    }

    if (g_file_info_has_attribute(gioInfo, G_FILE_ATTRIBUTE_FILESYSTEM_USED)) {
        quint64 used = g_file_info_get_attribute_uint64(gioInfo, G_FILE_ATTRIBUTE_FILESYSTEM_USED);
        return static_cast<qint64>(bytesTotal - used);
    } else {
        qInfo() << "file do not support G_FILE_ATTRIBUTE_FILESYSTEM_USED, returns max of qint64";
        return std::numeric_limits<qint64>::max();
    }
}

bool dfmio::DFMUtils::supportTrash(const QUrl &url)
{
    if (!url.isValid())
        return false;

    auto path = url.path();
    GStatBuf file_stat, home_stat;
    if (g_stat (path.toStdString().data(), &file_stat) != 0)
        return false;

    const char *homedir = g_get_home_dir ();
    g_stat (homedir, &home_stat);
    // 和当前用的主目录在同一挂载点
    if (file_stat.st_dev == home_stat.st_dev)
        return true;

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(url);
    g_autofree char *path1 = g_file_get_path(gfile);
    if (!path1)
        return false;
    g_autoptr(GUnixMountEntry) mount = g_unix_mount_for(path1, nullptr);
    if (mount == nullptr || g_unix_mount_is_system_internal (mount))
        return false;

    return true;
}

bool DFMUtils::isGvfsFile(const QUrl &url)
{
    if (!url.isValid())
        return false;

    const QString &path = url.toLocalFile();
    static const QString *gvfsMatch = new QString{ "(^/run/user/\\d+/gvfs/|^/root/.gvfs/|^/media/[\\s\\S]*/smbmounts)" };
    // TODO(xust) /media/$USER/smbmounts might be changed in the future.
    QRegularExpression re { *gvfsMatch };
    QRegularExpressionMatch match { re.match(path) };
    return match.hasMatch();
}

bool DFMUtils::isInvalidCodecByPath(const char *path)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::ConverterState stat;
    auto codec = QTextCodec::codecForLocale();

    auto str = codec->toUnicode(path, static_cast<int>(strlen(path)), &stat);
    return stat.invalidChars;
#else
    QByteArray pathA(path);
    return !pathA.isValidUtf8();
#endif
}

QMap<QString, QString> DFMUtils::fstabBindInfo()
{
    static QMutex mutex;
    static QMap<QString, QString> table;
    struct stat statInfo;
    int result = stat("/etc/fstab", &statInfo);

    QMutexLocker locker(&mutex);
    if (0 == result) {
        static quint32 lastModify = 0;
        if (lastModify != statInfo.st_mtime) {
            lastModify = static_cast<quint32>(statInfo.st_mtime);
            table.clear();
            struct fstab *fs;

            setfsent();
            while ((fs = getfsent()) != nullptr) {
                QString mntops(fs->fs_mntops);
                if (mntops.contains("bind"))
                    table.insert(fs->fs_spec, fs->fs_file);
            }
            endfsent();
        }
    }

    return table;
}

bool DFMUtils::compareFileName(const QString &str1, const QString &str2)
{
    return DLocalHelper::compareByStringEx(str1, str2);
}
