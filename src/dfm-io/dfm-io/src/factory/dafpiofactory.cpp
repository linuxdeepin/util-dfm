// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dafpiofactory.h"
#include "factory/dafpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DAfpIOFactoryPrivate::DAfpIOFactoryPrivate(DAfpIOFactory *q)
    : q(q)
{
}

DAfpIOFactoryPrivate::~DAfpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DAfpIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DAfpIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DAfpIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DAfpIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DAfpIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DAfpIOFactory::DAfpIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DAfpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DAfpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DAfpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DAfpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DAfpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DAfpIOFactory::createEnumerator, this));
}

DAfpIOFactory::~DAfpIOFactory()
{
}

QSharedPointer<DFileInfo> DAfpIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DAfpIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DAfpIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DAfpIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DAfpIOFactory::createOperator() const
{
    return d->createOperator();
}
