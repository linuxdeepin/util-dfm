// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILE_P_H
#define DFILE_P_H

#include <dfm-io/dfile.h>

#include <QPointer>

#include <gio/gio.h>

BEGIN_IO_NAMESPACE

class DFile;

class DFilePrivate : public QObject
{
public:
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
        QPointer<DFilePrivate> me;
    } ReadAllAsyncOp;

    typedef struct
    {
        DFile::WriteCallbackFunc callback;
        gpointer userData;
    } WriteAsyncOp;

    typedef struct
    {
        DFileFuture *future = nullptr;
        QPointer<DFilePrivate> me;
    } NormalFutureAsyncOp;

    typedef struct
    {
        QByteArray data;
        DFileFuture *future = nullptr;
        QPointer<DFilePrivate> me;
    } ReadAllAsyncFutureOp;

public:
    explicit DFilePrivate(DFile *q);
    ~DFilePrivate();
    void setError(DFMIOError error);
    void setErrorFromGError(GError *gerror);
    void checkAndResetCancel();
    GInputStream *inputStream();
    GOutputStream *outputStream();
    DFile::Permissions permissionsFromGFileInfo(GFileInfo *gfileinfo);
    bool checkOpenFlags(DFile::OpenFlags *modeIn);
    quint32 buildPermissions(DFile::Permissions permission);

    bool doOpen(DFile::OpenFlags mode);
    bool doClose();
    QByteArray doReadAll();
    qint64 doWrite(const char *data, qint64 maxSize);
    qint64 doWrite(const char *data);
    qint64 doWrite(const QByteArray &data);

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

public:
    DFile *q { nullptr };
    GIOStream *ioStream { nullptr };
    GInputStream *iStream { nullptr };
    GOutputStream *oStream { nullptr };
    GCancellable *cancellable { nullptr };
    DFMIOError error;
    QByteArray readAllAsyncRet;
    QUrl uri;
    bool isOpen { false };
};

END_IO_NAMESPACE

#endif   // DFILE_P_H
