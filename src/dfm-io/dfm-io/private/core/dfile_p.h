/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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

    DFile::SeekFunc seekFunc = nullptr;
    DFile::PosFunc posFunc = nullptr;
    DFile::FlushFunc flushFunc = nullptr;
    DFile::SizeFunc sizeFunc = nullptr;
    DFile::ExistsFunc existsFunc = nullptr;
    DFile::PermissionFunc permissionFunc = nullptr;
    DFile::SetPermissionFunc setPermissionsFunc = nullptr;
    DFile::LastErrorFunc lastErrorFunc = nullptr;
    DFile::SetErrorFunc setErrorFunc = nullptr;
};

END_IO_NAMESPACE

#endif   // DFILE_P_H
