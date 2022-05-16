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
#include "core/dfile_p.h"
#include "local/dlocalhelper.h"

#include <gio/gio.h>

#include <QDebug>

USING_IO_NAMESPACE

DFilePrivate::DFilePrivate(DFile *q)
    : q(q)
{
}

DFilePrivate::~DFilePrivate()
{
}

void DFilePrivate::setError(DFMIOError error)
{
    if (setErrorFunc)
        setErrorFunc(error);
}

DFile::DFile(const QUrl &uri)
    : d(new DFilePrivate(this))
{
    d->uri = uri;
}

DFile::~DFile()
{
}

bool DFile::open(DFile::OpenFlags mode)
{
    if (!d->openFunc)
        return false;

    d->isOpen = d->openFunc(mode);

    return d->isOpen;
}

bool DFile::close()
{
    if (!d->closeFunc)
        return false;

    if (d->isOpen) {
        if (d->closeFunc())
            d->isOpen = false;
        else
            return false;
    }

    return true;
}

qint64 DFile::read(char *data, qint64 maxSize)
{
    if (!d->readFunc)
        return -1;

    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return -1;
    }

    return d->readFunc(data, maxSize);
}

QByteArray DFile::read(qint64 maxSize)
{
    if (!d->readQFunc)
        return QByteArray();

    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return QByteArray();
    }

    return d->readQFunc(maxSize);
}

QByteArray DFile::readAll()
{
    if (!d->readAllFunc)
        return QByteArray();

    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return QByteArray();
    }

    return d->readAllFunc();
}

void DFile::readAsync(char *data, qint64 maxSize, int ioPriority, DFile::ReadCallbackFunc func, void *userData)
{
    if (!d->readFuncAsync)
        return;
    d->readFuncAsync(data, maxSize, ioPriority, func, userData);
}

void DFile::readQAsync(qint64 maxSize, int ioPriority, DFile::ReadQCallbackFunc func, void *userData)
{
    if (!d->readQFuncAsync)
        return;
    d->readQFuncAsync(maxSize, ioPriority, func, userData);
}

void DFile::readAllAsync(int ioPriority, DFile::ReadAllCallbackFunc func, void *userData)
{
    if (!d->readAllFuncAsync)
        return;
    d->readAllFuncAsync(ioPriority, func, userData);
}

qint64 DFile::write(const char *data, qint64 len)
{
    if (!d->writeFunc)
        return -1;

    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return -1;
    }

    return d->writeFunc(data, len);
}

qint64 DFile::write(const char *data)
{
    if (!d->writeAllFunc)
        return -1;

    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return -1;
    }

    return d->writeAllFunc(data);
}

qint64 DFile::write(const QByteArray &byteArray)
{
    if (!d->writeQFunc)
        return -1;

    if (!d->isOpen) {
        d->setError(DFMIOError(DFM_IO_ERROR_OPEN_FAILED));
        return -1;
    }

    return d->writeQFunc(byteArray);
}

void DFile::writeAsync(const char *data, qint64 len, int ioPriority, DFile::WriteCallbackFunc func, void *userData)
{
    if (!d->writeFuncAsync)
        return;
    d->writeFuncAsync(data, len, ioPriority, func, userData);
}

void DFile::writeAllAsync(const char *data, int ioPriority, DFile::WriteAllCallbackFunc func, void *userData)
{
    if (!d->writeAllFuncAsync)
        return;
    d->writeAllFuncAsync(data, ioPriority, func, userData);
}

void DFile::writeQAsync(const QByteArray &byteArray, int ioPriority, DFile::WriteQCallbackFunc func, void *userData)
{
    if (!d->writeQFuncAsync)
        return;
    d->writeQFuncAsync(byteArray, ioPriority, func, userData);
}

bool DFile::seek(qint64 pos, SeekType type)
{
    if (!d->seekFunc)
        return -1;

    return d->seekFunc(pos, type);
}

qint64 DFile::pos()
{
    if (!d->posFunc)
        return -1;

    return d->posFunc();
}

bool DFile::flush()
{
    if (!d->flushFunc)
        return -1;

    return d->flushFunc();
}

qint64 DFile::size()
{
    if (!d->sizeFunc)
        return -1;

    return d->sizeFunc();
}

bool DFile::exists()
{
    if (!d->existsFunc)
        return -1;

    return d->existsFunc();
}

DFile::Permissions DFile::permissions() const
{
    if (d->permissionFunc)
        return d->permissionFunc();
    return DFile::Permission::kNoPermission;
}

bool DFile::setPermissions(DFile::Permissions permission)
{
    if (!d->setPermissionsFunc)
        return false;
    return d->setPermissionsFunc(permission);
}

void DFile::registerOpen(const OpenFunc &func)
{
    d->openFunc = func;
}

void DFile::registerClose(const CloseFunc &func)
{
    d->closeFunc = func;
}

void DFile::registerRead(const ReadFunc &func)
{
    d->readFunc = func;
}

void DFile::registerReadQ(const DFile::ReadQFunc &func)
{
    d->readQFunc = func;
}

void DFile::registerReadAll(const DFile::ReadAllFunc &func)
{
    d->readAllFunc = func;
}

void DFile::registerReadAsync(const DFile::ReadFuncAsync &func)
{
    d->readFuncAsync = func;
}

void DFile::registerReadQAsync(const DFile::ReadQFuncAsync &func)
{
    d->readQFuncAsync = func;
}

void DFile::registerReadAllAsync(const DFile::ReadAllFuncAsync &func)
{
    d->readAllFuncAsync = func;
}

void DFile::registerWrite(const WriteFunc &func)
{
    d->writeFunc = func;
}

void DFile::registerWriteAll(const DFile::WriteAllFunc &func)
{
    d->writeAllFunc = func;
}

void DFile::registerWriteQ(const DFile::WriteQFunc &func)
{
    d->writeQFunc = func;
}

void DFile::registerWriteAsync(const DFile::WriteFuncAsync &func)
{
    d->writeFuncAsync = func;
}

void DFile::registerWriteAllAsync(const DFile::WriteAllFuncAsync &func)
{
    d->writeAllFuncAsync = func;
}

void DFile::registerWriteQAsync(const DFile::WriteQFuncAsync &func)
{
    d->writeQFuncAsync = func;
}

void DFile::registerSeek(const SeekFunc &func)
{
    d->seekFunc = func;
}

void DFile::registerPos(const DFile::PosFunc &func)
{
    d->posFunc = func;
}

void DFile::registerFlush(const DFile::FlushFunc &func)
{
    d->flushFunc = func;
}

void DFile::registerSize(const DFile::SizeFunc &func)
{
    d->sizeFunc = func;
}

void DFile::registerExists(const DFile::ExistsFunc &func)
{
    d->existsFunc = func;
}

void DFile::registerPermissions(const DFile::PermissionFunc &func)
{
    d->permissionFunc = func;
}

void DFile::registerSetPermissions(const DFile::SetPermissionFunc &func)
{
    d->setPermissionsFunc = func;
}

void DFile::registerLastError(const DFile::LastErrorFunc &func)
{
    d->lastErrorFunc = func;
}

void DFile::registerSetError(const DFile::SetErrorFunc &func)
{
    d->setErrorFunc = func;
}

QUrl DFile::uri() const
{
    return d->uri;
}

bool DFile::isOpen()
{
    return d->isOpen;
}

DFMIOError DFile::lastError() const
{
    if (!d->lastErrorFunc)
        return DFMIOError();

    return d->lastErrorFunc();
}
