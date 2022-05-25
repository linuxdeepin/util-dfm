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
#include "core/doperator_p.h"

USING_IO_NAMESPACE

DOperatorPrivate::DOperatorPrivate(DOperator *q)
    : q(q)
{
}

DOperatorPrivate::~DOperatorPrivate()
{
}

DOperator::DOperator(const QUrl &uri)
    : d(new DOperatorPrivate(this))
{
    d->uri = uri;
}

DOperator::~DOperator()
{
}

QUrl DOperator::uri() const
{
    return d->uri;
}

bool DOperator::renameFile(const QString &newName)
{
    if (!d->renameFileFunc)
        return false;

    return d->renameFileFunc(newName);
}

bool DOperator::renameFile(const QUrl &toUrl)
{
    if (!d->renameFileByStdFunc)
        return false;
    return d->renameFileByStdFunc(toUrl);
}

bool DOperator::copyFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func, void *progressCallbackData)
{
    if (!d->copyFileFunc)
        return false;

    return d->copyFileFunc(destUri, flag, func, progressCallbackData);
}

bool DOperator::moveFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func, void *progressCallbackData)
{
    if (!d->moveFileFunc)
        return false;

    return d->moveFileFunc(destUri, flag, func, progressCallbackData);
}

void DOperator::renameFileAsync(const QString &newName, int ioPriority, FileOperateCallbackFunc func, void *progressCallbackData)
{
    if (!d->renameFileFuncAsync)
        return;

    d->renameFileFuncAsync(newName, ioPriority, func, progressCallbackData);
}

void DOperator::copyFileAsync(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc progressfunc, void *progressCallbackData,
                              int ioPriority, FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->copyFileFuncAsync)
        return;

    d->copyFileFuncAsync(destUri, flag, progressfunc, progressCallbackData, ioPriority, operatefunc, userData);
}

void DOperator::moveFileAsync(const QUrl &destUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData,
                              int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->moveFileFuncAsync)
        return;

    d->moveFileFuncAsync(destUri, flag, func, progressCallbackData, ioPriority, operatefunc, userData);
}

bool DOperator::trashFile()
{
    if (!d->trashFileFunc)
        return false;

    return d->trashFileFunc();
}

bool DOperator::deleteFile()
{
    if (!d->deleteFileFunc)
        return false;

    return d->deleteFileFunc();
}

bool DOperator::restoreFile(ProgressCallbackFunc func, void *progressCallbackData)
{
    if (!d->restoreFileFunc)
        return false;

    return d->restoreFileFunc(func, progressCallbackData);
}

void DOperator::trashFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->trashFileFuncAsync)
        return;

    d->trashFileFuncAsync(ioPriority, operatefunc, userData);
}

void DOperator::deleteFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->deleteFileFuncAsync)
        return;

    d->deleteFileFuncAsync(ioPriority, operatefunc, userData);
}

void DOperator::restoreFileAsync(DOperator::ProgressCallbackFunc func, void *progressCallbackData, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->restoreFileFuncAsync)
        return;

    d->restoreFileFuncAsync(func, progressCallbackData, ioPriority, operatefunc, userData);
}

bool DOperator::touchFile()
{
    if (!d->touchFileFunc)
        return false;

    return d->touchFileFunc();
}

bool DOperator::makeDirectory()
{
    if (!d->makeDirectoryFunc)
        return false;

    return d->makeDirectoryFunc();
}

bool DOperator::createLink(const QUrl &link)
{
    if (!d->createLinkFunc)
        return false;

    return d->createLinkFunc(link);
}

void DOperator::touchFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->touchFileFuncAsync)
        return;

    d->touchFileFuncAsync(ioPriority, operatefunc, userData);
}

void DOperator::makeDirectoryAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->makeDirectoryFuncAsync)
        return;

    d->makeDirectoryFuncAsync(ioPriority, operatefunc, userData);
}

void DOperator::createLinkAsync(const QUrl &link, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    if (!d->createLinkFuncAsync)
        return;

    d->createLinkFuncAsync(link, ioPriority, operatefunc, userData);
}

bool DOperator::setFileInfo(const DFileInfo &fileInfo)
{
    if (!d->setFileInfoFunc)
        return false;

    return d->setFileInfoFunc(fileInfo);
}

bool DOperator::cancel()
{
    if (!d->cancelFunc)
        return false;

    return d->cancelFunc();
}

DFMIOError DOperator::lastError() const
{
    if (!d->lastErrorFunc)
        return DFMIOError();

    return d->lastErrorFunc();
}

void DOperator::registerRenameFile(const DOperator::RenameFileFunc &func)
{
    d->renameFileFunc = func;
}

void DOperator::registerRenameFileByStd(const DOperator::RenameFileByStdFunc &func)
{
    d->renameFileByStdFunc = func;
}

void DOperator::registerCopyFile(const DOperator::CopyFileFunc &func)
{
    d->copyFileFunc = func;
}

void DOperator::registerMoveFile(const DOperator::MoveFileFunc &func)
{
    d->moveFileFunc = func;
}

void DOperator::registerRenameFileAsync(const DOperator::RenameFileFuncAsync &func)
{
    d->renameFileFuncAsync = func;
}

void DOperator::registerCopyFileAsync(const DOperator::CopyFileFuncAsync &func)
{
    d->copyFileFuncAsync = func;
}

void DOperator::registerMoveFileAsync(const DOperator::MoveFileFuncAsync &func)
{
    d->moveFileFuncAsync = func;
}

void DOperator::registerTrashFile(const DOperator::TrashFileFunc &func)
{
    d->trashFileFunc = func;
}

void DOperator::registerDeleteFile(const DOperator::DeleteFileFunc &func)
{
    d->deleteFileFunc = func;
}

void DOperator::registerRestoreFile(const DOperator::RestoreFileFunc &func)
{
    d->restoreFileFunc = func;
}

void DOperator::registerTrashFileAsync(const DOperator::TrashFileFuncAsync &func)
{
    d->trashFileFuncAsync = func;
}

void DOperator::registerDeleteFileAsync(const DOperator::DeleteFileFuncAsync &func)
{
    d->deleteFileFuncAsync = func;
}

void DOperator::registerRestoreFileAsync(const DOperator::RestoreFileFuncAsync &func)
{
    d->restoreFileFuncAsync = func;
}

void DOperator::registerTouchFile(const DOperator::TouchFileFunc &func)
{
    d->touchFileFunc = func;
}

void DOperator::registerMakeDirectory(const DOperator::MakeDirectoryFunc &func)
{
    d->makeDirectoryFunc = func;
}

void DOperator::registerCreateLink(const DOperator::CreateLinkFunc &func)
{
    d->createLinkFunc = func;
}

void DOperator::registerTouchFileAsync(const DOperator::TouchFileFuncAsync &func)
{
    d->touchFileFuncAsync = func;
}

void DOperator::registerMakeDirectoryAsync(const DOperator::MakeDirectoryFuncAsync &func)
{
    d->makeDirectoryFuncAsync = func;
}

void DOperator::registerCreateLinkAsync(const DOperator::CreateLinkFuncAsync &func)
{
    d->createLinkFuncAsync = func;
}

void DOperator::registerSetFileInfo(const DOperator::SetFileInfoFunc &func)
{
    d->setFileInfoFunc = func;
}

void DOperator::registerCancel(const DOperator::CancelFunc &func)
{
    d->cancelFunc = func;
}

void DOperator::registerLastError(const DOperator::LastErrorFunc &func)
{
    d->lastErrorFunc = func;
}
