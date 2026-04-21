// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
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
    qint64 read(char *data, qint64 maxSize);
    QByteArray read(qint64 maxSize);
    qint64 pos() const;
    bool seek(qint64 pos, DFile::SeekType type = DFile::SeekType::kBegin) const;

    bool doOpenBySys(const int model, const int permissions);
    bool doCloseBySys();
    qint64 doWriteBySys(const char *data, const qint64 maxSize);
    qint64 doWriteBySys(const char *data);
    qint64 doWriteBySys(const QByteArray &data);
    qint64 readBySys(char *data, qint64 maxSize);
    QByteArray readBySys(qint64 maxSize);
    qint64 posBySys() const;
    bool seekBySys(const qint64 pos, const DFile::SeekType type = DFile::SeekType::kBegin) const;

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
    DFile::FileCopyType fileCopyType { DFile::FileCopyType::kCopyTypeByGioStream };
    DFile::FileCopySyncType fileSyncType { DFile::FileCopySyncType::kSyncByGioStream };
    bool isOpen { false };
    int fileFd { -1 };
};

END_IO_NAMESPACE

#endif   // DFILE_P_H
