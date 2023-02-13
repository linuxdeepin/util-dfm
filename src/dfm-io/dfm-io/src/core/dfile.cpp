// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

bool DFile::cancel()
{
    if (!d->cancelFunc)
        return false;

    return d->cancelFunc();
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

bool DFile::seek(qint64 pos, SeekType type) const
{
    if (!d->seekFunc)
        return -1;

    return d->seekFunc(pos, type);
}

qint64 DFile::pos() const
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

qint64 DFile::size() const
{
    if (!d->sizeFunc)
        return -1;

    return d->sizeFunc();
}

bool DFile::exists() const
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

void DFile::registerCancel(const DFile::CancelFunc &func)
{
    d->cancelFunc = func;
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

void DFile::registerOpenAsyncFuture(const DFile::OpenAsyncFuncFuture &func)
{
    d->openAsyncFuncFuture = func;
}

void DFile::registerCloseAsyncFuture(const DFile::CloseAsyncFuncFuture &func)
{
    d->closeAsyncFuncFuture = func;
}

void DFile::registerReadAsyncFuture(const DFile::ReadAsyncFuncFuture &func)
{
    d->readAsyncFuncFuture = func;
}

void DFile::registerReadAllAsyncFuture(const DFile::ReadAllAsyncFuncFuture &func)
{
    d->readAllAsyncFuncFuture = func;
}

void DFile::registerWriteAsyncFuture(const DFile::WriteAsyncFuncFuture &func)
{
    d->writeAsyncFuncFuture = func;
}

void DFile::registerWriteAllAsyncFuture(const DFile::WriteAllAsyncFuncFuture &func)
{
    d->writeAllAsyncFuncFuture = func;
}

void DFile::registerFlushAsyncFuture(const DFile::FlushAsyncFuncFuture &func)
{
    d->flushAsyncFuncFuture = func;
}

void DFile::registerSizeAsyncFuture(const DFile::SizeAsyncFuncFuture &func)
{
    d->sizeAsyncFuncFuture = func;
}

void DFile::registerExistsAsyncFuture(const DFile::ExistsAsyncFuncFuture &func)
{
    d->existsAsyncFuncFuture = func;
}

void DFile::registerPermissionsAsyncFuture(const DFile::PermissionsAsyncFuncFuture &func)
{
    d->permissionsAsyncFuncFuture = func;
}

void DFile::registerSetPermissionsAsyncFuture(const DFile::SetPermissionsAsyncFuncFuture &func)
{
    d->setPermissionsAsyncFuncFuture = func;
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

DFileFuture *DFile::openAsync(OpenFlags mode, int ioPriority, QObject *parent)
{
    if (d->openAsyncFuncFuture)
        return d->openAsyncFuncFuture(mode, ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::closeAsync(int ioPriority, QObject *parent)
{
    if (d->closeAsyncFuncFuture)
        return d->closeAsyncFuncFuture(ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::readAsync(qint64 maxSize, int ioPriority, QObject *parent)
{
    if (d->readAsyncFuncFuture)
        return d->readAsyncFuncFuture(maxSize, ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::readAllAsync(int ioPriority, QObject *parent)
{
    if (d->readAllAsyncFuncFuture)
        return d->readAllAsyncFuncFuture(ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::writeAsync(const QByteArray &data, qint64 len, int ioPriority, QObject *parent)
{
    if (d->writeAsyncFuncFuture)
        return d->writeAsyncFuncFuture(data, len, ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::writeAsync(const QByteArray &data, int ioPriority, QObject *parent)
{
    if (d->writeAllAsyncFuncFuture)
        return d->writeAllAsyncFuncFuture(data, ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::flushAsync(int ioPriority, QObject *parent)
{
    if (d->flushAsyncFuncFuture)
        return d->flushAsyncFuncFuture(ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::sizeAsync(int ioPriority, QObject *parent)
{
    if (d->sizeAsyncFuncFuture)
        return d->sizeAsyncFuncFuture(ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::existsAsync(int ioPriority, QObject *parent)
{
    if (d->existsAsyncFuncFuture)
        return d->existsAsyncFuncFuture(ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::permissionsAsync(int ioPriority, QObject *parent)
{
    if (d->permissionsAsyncFuncFuture)
        return d->permissionsAsyncFuncFuture(ioPriority, parent);
    return nullptr;
}

DFileFuture *DFile::setPermissionsAsync(Permissions permission, int ioPriority, QObject *parent)
{
    if (d->setPermissionsAsyncFuncFuture)
        return d->setPermissionsAsyncFuncFuture(permission, ioPriority, parent);
    return nullptr;
}
