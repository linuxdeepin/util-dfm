// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dafciofactory.h"
#include "factory/dafciofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DAfcIOFactoryPrivate::DAfcIOFactoryPrivate(DAfcIOFactory *q)
    : q(q)
{
}

DAfcIOFactoryPrivate::~DAfcIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DAfcIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DAfcIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DAfcIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DAfcIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DAfcIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DAfcIOFactory::DAfcIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DAfcIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DAfcIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DAfcIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DAfcIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DAfcIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DAfcIOFactory::createEnumerator, this));
}

DAfcIOFactory::~DAfcIOFactory()
{
}

QSharedPointer<DFileInfo> DAfcIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DAfcIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DAfcIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DAfcIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DAfcIOFactory::createOperator() const
{
    return d->createOperator();
}
