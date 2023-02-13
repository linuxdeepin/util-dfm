// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/diofactory.h"
#include "error.h"
#include "core/diofactory_p.h"

USING_IO_NAMESPACE

DIOFactoryPrivate::DIOFactoryPrivate(DIOFactory *q)
    : q(q)
{
}

DIOFactoryPrivate::~DIOFactoryPrivate()
{
}

DIOFactory::DIOFactory(const QUrl &uri)
    : d(new DIOFactoryPrivate(this))
{
    d->uri = uri;
}

DIOFactory::~DIOFactory()
{
}

void DIOFactory::setUri(const QUrl &uri)
{
    if (!d)
        return;

    d->uri = uri;
}

QUrl DIOFactory::uri() const
{
    if (!d)
        return QUrl();

    return d->uri;
}

QSharedPointer<DFileInfo> DIOFactory::createFileInfo(const char *attributes /*= "*"*/,
                                                     const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/) const
{
    if (!d->createFileInfoFunc)
        return nullptr;

    return d->createFileInfoFunc(attributes, flag);
}

QSharedPointer<DFile> DIOFactory::createFile() const
{
    if (!d->createFileFunc)
        return nullptr;

    return d->createFileFunc();
}

QSharedPointer<DEnumerator> DIOFactory::createEnumerator(const QStringList &nameFilters, DEnumerator::DirFilters filters, DEnumerator::IteratorFlags flags) const
{
    if (!d->createEnumeratorFunc)
        return nullptr;

    return d->createEnumeratorFunc(nameFilters, filters, flags);
}

QSharedPointer<DWatcher> DIOFactory::createWatcher() const
{
    if (!d->createWatcherFunc)
        return nullptr;

    return d->createWatcherFunc();
}

QSharedPointer<DOperator> DIOFactory::createOperator() const
{
    if (!d->createOperatorFunc)
        return nullptr;

    return d->createOperatorFunc();
}

void DIOFactory::registerCreateFileInfo(const DIOFactory::CreateFileInfoFunc &func)
{
    d->createFileInfoFunc = func;
}

void DIOFactory::registerCreateFile(const DIOFactory::CreateFileFunc &func)
{
    d->createFileFunc = func;
}

void DIOFactory::registerCreateEnumerator(const DIOFactory::CreateEnumeratorFunc &func)
{
    d->createEnumeratorFunc = func;
}

void DIOFactory::registerCreateWatcher(const DIOFactory::CreateWatcherFunc &func)
{
    d->createWatcherFunc = func;
}

void DIOFactory::registerCreateOperator(const DIOFactory::CreateOperatorFunc &func)
{
    d->createOperatorFunc = func;
}

DFMIOError DIOFactory::lastError() const
{
    if (!d)
        return DFMIOError();

    return d->error;
}
