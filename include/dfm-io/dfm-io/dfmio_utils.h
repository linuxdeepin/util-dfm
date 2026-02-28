// SPDX-FileCopyrightText: 2021 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFMIO_UTILS_H
#define DFMIO_UTILS_H

#include <dfm-io/dfmio_global.h>

#include <QString>
#include <QObject>

class QUrl;

BEGIN_IO_NAMESPACE

class DEnumeratorFuture;
enum class DGlibUserDirectory : quint8 {
    kUserDirectoryDesktop,
    kUserDirectoryDocuments,
    kUserDirectoryDownload,
    kUserDirectoryMusic,
    kUserDirectoryPictures,
    kUserDirectoryPublicShare,
    kUserDirectoryTemplates,
    kUserDirectoryVideos,
    kUserNDirectories
};
Q_ENUMS(DGlibUserDirectory);

class DFMUtils
{

public:
    static bool fileUnmountable(const QString &path);
    static QString devicePathFromUrl(const QUrl &url);
    static QString deviceNameFromUrl(const QUrl &url);
    static QString fsTypeFromUrl(const QUrl &url);
    static QString mountPathFromUrl(const QUrl &url);
    static QUrl directParentUrl(const QUrl &url, const bool localFirst = true);
    static bool fileIsRemovable(const QUrl &url);
    static QSet<QString> hideListFromUrl(const QUrl &url);
    /*
     * build file path, parameter should endwith "nullptr"
     * e.g.: buildPath("/", "Desktop", "nullptr"), then return "/Desktop"
     */
    static QString buildFilePath(const char *segment, ...);
    static QStringList systemDataDirs();
    static QString userSpecialDir(DGlibUserDirectory userDirectory);
    static QString userDataDir();
    static QString bindPathTransform(const QString &path, bool toDevice);
    static int dirFfileCount(const QUrl &url);
    static QUrl bindUrlTransform(const QUrl &url);
    static QString BackslashPathToNormal(const QString &trash);
    static QString normalPathToBackslash(const QString &normal);
    // 通过迭代器去获取回收站数量，并做相同挂载点过滤
    static DEnumeratorFuture *asyncTrashCount();
    static int syncTrashCount();
    static qint64 deviceBytesFree(const QUrl &url);
    static bool supportTrash(const QUrl &url);
    static bool isGvfsFile(const QUrl &url);
    // String comparison function for file names
    static bool compareFileName(const QString &str1, const QString &str2);
    static bool isInvalidCodecByPath(const char *path);

private:
    static QMap<QString, QString>
    fstabBindInfo();
};

END_IO_NAMESPACE

#endif   // DFMIO_UTILS_H
