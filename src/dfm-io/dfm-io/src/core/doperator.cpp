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
    : q_ptr(q)
{

}

DOperatorPrivate::~DOperatorPrivate()
{

}

DOperator::DOperator(const QUrl &uri)
{
    Q_D(DOperator);
    d->uri = uri;
}

DOperator::~DOperator()
{

}

QUrl DOperator::uri() const
{
    Q_D(const DOperator);

    return d->uri;
}

DFM_VIRTUAL bool DOperator::renameFile(const QString &newName)
{
    Q_D(DOperator);

    if (!d->renameFileFunc)
        return false;

    return d->renameFileFunc(newName);
}

DFM_VIRTUAL bool DOperator::copyFile(const QUrl &destUri, DOperator::CopyFlag flag)
{
    Q_D(DOperator);

    if (!d->copyFileFunc)
        return false;

    return d->copyFileFunc(destUri, flag);
}

DFM_VIRTUAL bool DOperator::moveFile(const QUrl &destUri, DOperator::CopyFlag flag)
{
    Q_D(DOperator);

    if (!d->moveFileFunc)
        return false;

    return d->moveFileFunc(destUri, flag);
}

DFM_VIRTUAL bool DOperator::trashFile()
{
    Q_D(DOperator);

    if (!d->trashFileFunc)
        return false;

    return d->trashFileFunc();
}

DFM_VIRTUAL bool DOperator::deleteFile()
{
    Q_D(DOperator);

    if (!d->deleteFileFunc)
        return false;

    return d->deleteFileFunc();
}

DFM_VIRTUAL bool DOperator::restoreFile()
{
    Q_D(DOperator);

    if (!d->restoreFileFunc)
        return false;

    return d->restoreFileFunc();
}

DFM_VIRTUAL bool DOperator::touchFile()
{
    Q_D(DOperator);

    if (!d->touchFileFunc)
        return false;

    return d->touchFileFunc();
}

DFM_VIRTUAL bool DOperator::makeDirectory()
{
    Q_D(DOperator);

    if (!d->makeDirectoryFunc)
        return false;

    return d->makeDirectoryFunc();
}

DFM_VIRTUAL bool DOperator::createLink(const QUrl &link)
{
    Q_D(DOperator);

    if (!d->createLinkFunc)
        return false;

    return d->createLinkFunc(link);
}

DFM_VIRTUAL bool DOperator::setFileInfo(const DFileInfo &fileInfo)
{
    Q_D(DOperator);

    if (!d->setFileInfoFunc)
        return false;

    return d->setFileInfoFunc(fileInfo);
}

void DOperator::registerRenameFile(const DOperator::RenameFileFunc &func)
{
    Q_D(DOperator);
    d->renameFileFunc = func;
}

void DOperator::registerCopyFile(const DOperator::CopyFileFunc &func)
{
    Q_D(DOperator);
    d->copyFileFunc = func;
}

void DOperator::registerMoveFile(const DOperator::MoveFileFunc &func)
{
    Q_D(DOperator);
    d->moveFileFunc = func;
}

void DOperator::registerTrashFile(const DOperator::TrashFileFunc &func)
{
    Q_D(DOperator);
    d->trashFileFunc = func;
}

void DOperator::registerDeleteFile(const DOperator::DeleteFileFunc &func)
{
    Q_D(DOperator);
    d->deleteFileFunc = func;
}

void DOperator::registerRestoreFile(const DOperator::RestoreFileFunc &func)
{
    Q_D(DOperator);
    d->restoreFileFunc = func;
}

void DOperator::registerTouchFile(const DOperator::TouchFileFunc &func)
{
    Q_D(DOperator);
    d->touchFileFunc = func;
}

void DOperator::registerMakeDirectory(const DOperator::MakeDirectoryFunc &func)
{
    Q_D(DOperator);
    d->makeDirectoryFunc = func;
}

void DOperator::registerCreateLink(const DOperator::CreateLinkFunc &func)
{
    Q_D(DOperator);
    d->createLinkFunc = func;
}

void DOperator::registerSetFileInfo(const DOperator::SetFileInfoFunc &func)
{
    Q_D(DOperator);
    d->setFileInfoFunc = func;
}

DFMIOError DOperator::lastError() const
{
    Q_D(const DOperator);

    if (!d)
        return DFMIOError();

    return d->error;
}
