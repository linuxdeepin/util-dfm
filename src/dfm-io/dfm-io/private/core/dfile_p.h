// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFILE_P_H
#define DFILE_P_H

#include "core/dfile.h"

BEGIN_IO_NAMESPACE

class DFile;

class DFilePrivate
{
public:
    explicit DFilePrivate(DFile *q);
    virtual ~DFilePrivate();

    void setError(DFMIOError error);

public:
    DFile *q = nullptr;
    QUrl uri;

    bool isOpen = false;

    DFile::OpenFunc openFunc = nullptr;
    DFile::CloseFunc closeFunc = nullptr;
    DFile::ReadFunc readFunc = nullptr;
    DFile::ReadQFunc readQFunc = nullptr;
    DFile::ReadAllFunc readAllFunc = nullptr;
    DFile::ReadFuncAsync readFuncAsync = nullptr;
    DFile::ReadQFuncAsync readQFuncAsync = nullptr;
    DFile::ReadAllFuncAsync readAllFuncAsync = nullptr;

    DFile::WriteFunc writeFunc = nullptr;
    DFile::WriteAllFunc writeAllFunc = nullptr;
    DFile::WriteQFunc writeQFunc = nullptr;
    DFile::WriteFuncAsync writeFuncAsync = nullptr;
    DFile::WriteAllFuncAsync writeAllFuncAsync = nullptr;
    DFile::WriteQFuncAsync writeQFuncAsync = nullptr;

    DFile::CancelFunc cancelFunc = nullptr;
    DFile::SeekFunc seekFunc = nullptr;
    DFile::PosFunc posFunc = nullptr;
    DFile::FlushFunc flushFunc = nullptr;
    DFile::SizeFunc sizeFunc = nullptr;
    DFile::ExistsFunc existsFunc = nullptr;
    DFile::PermissionFunc permissionFunc = nullptr;
    DFile::SetPermissionFunc setPermissionsFunc = nullptr;
    DFile::LastErrorFunc lastErrorFunc = nullptr;
    DFile::SetErrorFunc setErrorFunc = nullptr;

    // future
    DFile::OpenAsyncFuncFuture openAsyncFuncFuture = nullptr;
    DFile::CloseAsyncFuncFuture closeAsyncFuncFuture = nullptr;
    DFile::ReadAsyncFuncFuture readAsyncFuncFuture = nullptr;
    DFile::ReadAllAsyncFuncFuture readAllAsyncFuncFuture = nullptr;
    DFile::WriteAsyncFuncFuture writeAsyncFuncFuture = nullptr;
    DFile::WriteAllAsyncFuncFuture writeAllAsyncFuncFuture = nullptr;
    DFile::FlushAsyncFuncFuture flushAsyncFuncFuture = nullptr;
    DFile::SizeAsyncFuncFuture sizeAsyncFuncFuture = nullptr;
    DFile::ExistsAsyncFuncFuture existsAsyncFuncFuture = nullptr;
    DFile::PermissionsAsyncFuncFuture permissionsAsyncFuncFuture = nullptr;
    DFile::SetPermissionsAsyncFuncFuture setPermissionsAsyncFuncFuture = nullptr;
};

END_IO_NAMESPACE

#endif   // DFILE_P_H
