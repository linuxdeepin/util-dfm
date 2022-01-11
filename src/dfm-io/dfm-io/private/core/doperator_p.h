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

#include "core/doperator.h"

BEGIN_IO_NAMESPACE

class DOperatorPrivate
{
public:
    explicit DOperatorPrivate(DOperator *q);
    virtual ~DOperatorPrivate();

public:
    DOperator *q = nullptr;
    QUrl uri;

    DOperator::RenameFileFunc renameFileFunc = nullptr;
    DOperator::RenameFileByStdFunc renameFileByStdFunc = nullptr;
    DOperator::CopyFileFunc copyFileFunc = nullptr;
    DOperator::MoveFileFunc moveFileFunc = nullptr;
    DOperator::RenameFileFuncAsync renameFileFuncAsync = nullptr;
    DOperator::CopyFileFuncAsync copyFileFuncAsync = nullptr;
    DOperator::MoveFileFuncAsync moveFileFuncAsync = nullptr;

    DOperator::TrashFileFunc trashFileFunc = nullptr;
    DOperator::DeleteFileFunc deleteFileFunc = nullptr;
    DOperator::RestoreFileFunc restoreFileFunc = nullptr;
    DOperator::TrashFileFuncAsync trashFileFuncAsync = nullptr;
    DOperator::DeleteFileFuncAsync deleteFileFuncAsync = nullptr;
    DOperator::RestoreFileFuncAsync restoreFileFuncAsync = nullptr;

    DOperator::TouchFileFunc touchFileFunc = nullptr;
    DOperator::MakeDirectoryFunc makeDirectoryFunc = nullptr;
    DOperator::CreateLinkFunc createLinkFunc = nullptr;
    DOperator::TouchFileFuncAsync touchFileFuncAsync = nullptr;
    DOperator::MakeDirectoryFuncAsync makeDirectoryFuncAsync = nullptr;
    DOperator::CreateLinkFuncAsync createLinkFuncAsync = nullptr;

    DOperator::SetFileInfoFunc setFileInfoFunc = nullptr;

    DOperator::CancelFunc cancelFunc = nullptr;
    DOperator::LastErrorFunc lastErrorFunc = nullptr;
};

END_IO_NAMESPACE

#endif   // DOPERATOR_P_H
