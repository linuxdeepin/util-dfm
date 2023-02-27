// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOPERATOR_P_H
#define DOPERATOR_P_H

#include <dfm-io/doperator.h>

#include <gio/gio.h>

BEGIN_IO_NAMESPACE

class DOperatorPrivate
{
public:
    typedef struct
    {
        DOperator::FileOperateCallbackFunc callback;
        gpointer userData;
    } OperateFileOp;

    explicit DOperatorPrivate(DOperator *q);
    virtual ~DOperatorPrivate();

    void setErrorFromGError(GError *gerror);
    GFile *makeGFile(const QUrl &url);

    static void renameCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void copyCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void trashCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void deleteCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void touchCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void makeDirCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);

public:
    DOperator *q { nullptr };
    QUrl uri;
    GCancellable *gcancellable { nullptr };
    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DOPERATOR_P_H
