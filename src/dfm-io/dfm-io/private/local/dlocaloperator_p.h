// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DLOCALOPERATOR_P_H
#define DLOCALOPERATOR_P_H

#include "dfmio_global.h"
#include "core/doperator_p.h"

#include "gio/gio.h"

BEGIN_IO_NAMESPACE

class DLocalOperator;

class DLocalOperatorPrivate
{
public:
    explicit DLocalOperatorPrivate(DLocalOperator *q);
    ~DLocalOperatorPrivate();

    typedef struct
    {
        DOperator::FileOperateCallbackFunc callback;
        gpointer userData;
    } OperateFileOp;

    bool renameFile(const QString &new_name);
    bool renameFile(const QUrl &toUrl);
    bool copyFile(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    bool moveFile(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    void renameFileAsync(const QString &newName, int ioPriority = 0, DOperator::FileOperateCallbackFunc func = nullptr, void *userData = nullptr);
    void copyFileAsync(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void moveFileAsync(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    QString trashFile();
    bool deleteFile();
    bool restoreFile(DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    void trashFileAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void deleteFileAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void restoreFileAsync(DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                          int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    bool touchFile();
    bool makeDirectory();
    bool createLink(const QUrl &link);
    void touchFileAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void makeDirectoryAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void createLinkAsync(const QUrl &link, int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    bool setFileInfo(const DFileInfo &fileInfo);
    bool cancel();

    DFMIOError lastError();
    void setErrorFromGError(GError *gerror);

    GFile *makeGFile(const QUrl &url);

    static void renameCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void copyCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void trashCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void deleteCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void touchCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void makeDirCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);

private:
    DLocalOperator *q = nullptr;
    GCancellable *gcancellable = nullptr;

    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DLOCALOPERATOR_P_H
