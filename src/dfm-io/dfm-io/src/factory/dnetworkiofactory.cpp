// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dnetworkiofactory.h"
#include "factory/dnetworkiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DNetworkIOFactoryPrivate::DNetworkIOFactoryPrivate(DNetworkIOFactory *q)
    : q(q)
{
}

DNetworkIOFactoryPrivate::~DNetworkIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DNetworkIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DNetworkIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DNetworkIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DNetworkIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DNetworkIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DNetworkIOFactory::DNetworkIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DNetworkIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DNetworkIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DNetworkIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DNetworkIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DNetworkIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DNetworkIOFactory::createEnumerator, this));
}

DNetworkIOFactory::~DNetworkIOFactory()
{
}

QSharedPointer<DFileInfo> DNetworkIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DNetworkIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DNetworkIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DNetworkIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DNetworkIOFactory::createOperator() const
{
    return d->createOperator();
}
