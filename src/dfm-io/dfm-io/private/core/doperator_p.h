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
#ifndef DOPERATOR_P_H
#define DOPERATOR_P_H

#include "error.h"
#include "core/doperator.h"

BEGIN_IO_NAMESPACE

class DOperatorPrivate
{
    Q_DECLARE_PUBLIC(DOperator)
public:
    explicit DOperatorPrivate(DOperator *q);
    virtual ~DOperatorPrivate();

    /*virtual bool renameFile (const QString &new_name) = 0;
    virtual bool copyFile(const QUrl &dstUri, DOperator::CopyFlag flag) = 0;
    virtual bool moveFile(const QUrl &dstUri, DOperator::CopyFlag flag) = 0;

    virtual bool trashFile() = 0;
    virtual bool deleteFile() = 0;
    virtual bool restoreFile() = 0;

    virtual bool touchFile() = 0;
    virtual bool makeDirectory() = 0;
    virtual bool createLink(const QUrl &link) = 0;
    virtual bool setFileInfo(const DFileInfo &fileInfo) = 0;*/

public:
    DOperator *q_ptr;
    QUrl uri;

    DOperator::RenameFileFunc renameFileFunc = nullptr;
    DOperator::CopyFileFunc copyFileFunc = nullptr;
    DOperator::MoveFileFunc moveFileFunc = nullptr;

    DOperator::TrashFileFunc trashFileFunc = nullptr;
    DOperator::DeleteFileFunc deleteFileFunc = nullptr;
    DOperator::RestoreFileFunc restoreFileFunc = nullptr;

    DOperator::TouchFileFunc touchFileFunc = nullptr;
    DOperator::MakeDirectoryFunc makeDirectoryFunc = nullptr;
    DOperator::CreateLinkFunc createLinkFunc = nullptr;
    DOperator::SetFileInfoFunc setFileInfoFunc = nullptr;

    DFMIOError error;
};

END_IO_NAMESPACE

#endif // DOPERATOR_P_H
