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

#ifndef DLOCALFILE_P_H
#define DLOCALFILE_P_H

#include "dfmio_global.h"

#include "core/dfile_p.h"

#include "gio/gio.h"

#include <QPointer>

BEGIN_IO_NAMESPACE

class DLocalFile;

class DLocalFilePrivate : public QObject
{
public:
    explicit DLocalFilePrivate(DLocalFile *q);
    ~DLocalFilePrivate();

    typedef struct
    {
        DFile::ReadCallbackFunc callback;
        gpointer userData;
    } ReadAsyncOp;

    typedef struct
    {
        DFile::ReadQCallbackFunc callback;
        char *data;
        gpointer userData;
    } ReadQAsyncOp;

    typedef struct
    {
        char *data;
        int ioPriority;
        DFile::ReadAllCallbackFunc callback;
        gpointer userData;
        QPointer<DLocalFilePrivate> me;
    } ReadAllAsyncOp;

    typedef struct
    {
        DFile::WriteCallbackFunc callback;
        gpointer userData;
    } WriteAsyncOp;

    typedef struct
    {
        DFileFuture *future = nullptr;
        QPointer<DLocalFilePrivate> me;
    } NormalFutureAsyncOp;

    typedef struct
    {
        QByteArray data;
        DFileFuture *future = nullptr;
        QPointer<DLocalFilePrivate> me;
    } ReadAllAsyncFutureOp;

    bool open(DFile::OpenFlags mode);
    bool close();
    qint64 read(char *data, qint64 maxSize);
    QByteArray read(qint64 maxSize);
    QByteArray readAll();
    // async
    void readAsync(char *data, qint64 maxSize, int ioPriority = 0, DFile::ReadCallbackFunc func = nullptr, void *userData = nullptr);
    void readQAsync(qint64 maxSize, int ioPriority = 0, DFile::ReadQCallbackFunc func = nullptr, void *userData = nullptr);
    void readAllAsync(int ioPriority = 0, DFile::ReadAllCallbackFunc func = nullptr, void *userData = nullptr);

    qint64 write(const char *data, qint64 len);
    qint64 write(const char *data);
    qint64 write(const QByteArray &data);
    // async
    void writeAsync(const char *data, qint64 len, int ioPriority = 0, DFile::WriteCallbackFunc func = nullptr, void *userData = nullptr);
    void writeAllAsync(const char *data, int ioPriority = 0, DFile::WriteAllCallbackFunc func = nullptr, void *userData = nullptr);
    void writeQAsync(const QByteArray &byteArray, int ioPriority = 0, DFile::WriteQCallbackFunc func = nullptr, void *userData = nullptr);

    bool cancel();
    bool seek(qint64 pos, DFile::SeekType type = DFile::SeekType::kBegin);
    qint64 pos();
    bool flush();
    qint64 size();
    bool exists();
    DFile::Permissions permissions();
    bool setPermissions(DFile::Permissions permission);
    quint32 buildPermissions(DFile::Permissions permission);
    DFile::Permissions permissionsFromGFileInfo(GFileInfo *gfileinfo);

    // future
    [[nodiscard]] DFileFuture *openAsync(DFile::OpenFlags mode, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *closeAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *readAsync(qint64 maxSize, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *readAllAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *writeAsync(const QByteArray &data, qint64 len, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *writeAsync(const QByteArray &data, int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *flushAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *sizeAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *existsAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *permissionsAsync(int ioPriority, QObject *parent = nullptr);
    [[nodiscard]] DFileFuture *setPermissionsAsync(DFile::Permissions permission, int ioPriority, QObject *parent = nullptr);

    GInputStream *inputStream();
    GOutputStream *outputStream();

    DFMIOError lastError();
    void setError(DFMIOError error);
    void setErrorFromGError(GError *gerror);

    bool checkOpenFlags(DFile::OpenFlags *mode);

    static void readAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void readQAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void readAllAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void writeAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void permissionsAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void existsAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void sizeAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void flushAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void writeAsyncFutureCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void readAsyncFutureCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);

private:
    void checkAndResetCancel();

public:
    GIOStream *ioStream = nullptr;
    GInputStream *iStream = nullptr;
    GOutputStream *oStream = nullptr;
    GCancellable *cancellable = nullptr;

    DFMIOError error;

    DLocalFile *q = nullptr;
    QByteArray readAllAsyncRet;
};

END_IO_NAMESPACE

#endif   // DLOCALFILE_P_H
