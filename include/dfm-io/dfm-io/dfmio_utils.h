// SPDX-FileCopyrightText: 2021 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFMIO_UTILS_H
#define DFMIO_UTILS_H

#include <dfm-io/dfmio_global.h>

#include <QString>
#include <QObject>

class QUrl;

BEGIN_IO_NAMESPACE

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
    static QString fsTypeFromUrl(const QUrl &url);
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
};

END_IO_NAMESPACE

#endif   // DFMIO_UTILS_H
