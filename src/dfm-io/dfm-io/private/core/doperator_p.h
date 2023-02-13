// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
