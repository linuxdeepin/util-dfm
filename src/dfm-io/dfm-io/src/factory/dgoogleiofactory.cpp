// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dgoogleiofactory.h"
#include "factory/dgoogleiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DGoogleIOFactoryPrivate::DGoogleIOFactoryPrivate(DGoogleIOFactory *q)
    : q(q)
{
}

DGoogleIOFactoryPrivate::~DGoogleIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DGoogleIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DGoogleIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DGoogleIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DGoogleIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DGoogleIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DGoogleIOFactory::DGoogleIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DGoogleIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DGoogleIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DGoogleIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DGoogleIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DGoogleIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DGoogleIOFactory::createEnumerator, this));
}

DGoogleIOFactory::~DGoogleIOFactory()
{
}

QSharedPointer<DFileInfo> DGoogleIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DGoogleIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DGoogleIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DGoogleIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DGoogleIOFactory::createOperator() const
{
    return d->createOperator();
}
