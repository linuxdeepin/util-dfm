// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRASHFILEHELPER_H
#define TRASHFILEHELPER_H

#include <dfm-io/dfmio_global.h>

#include <QString>

#include <gio/gio.h>

BEGIN_IO_NAMESPACE
class TrashFileHelper
{
public:
    TrashFileHelper();
    static QString trashTargetPath(const QString &path, GFile *file);

private:
    static QString getParent(const QString &path, dev_t *parentDev);
    static dev_t getFileDevType(const QString &path);
    static QString findMountpoint(const QString &path, const dev_t dev);
    static QString trashDir(const QString &path, QString *topDir);
    static QString getTrashFilename(const char *basename, int id);
};
END_IO_NAMESPACE
#endif   // TRASHFILEHELPER_H
