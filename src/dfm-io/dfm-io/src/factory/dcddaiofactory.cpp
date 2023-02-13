// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dcddaiofactory.h"
#include "factory/dcddaiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DCddaIOFactoryPrivate::DCddaIOFactoryPrivate(DCddaIOFactory *q)
    : q(q)
{
}

DCddaIOFactoryPrivate::~DCddaIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DCddaIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DCddaIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DCddaIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DCddaIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DCddaIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DCddaIOFactory::DCddaIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DCddaIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DCddaIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DCddaIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DCddaIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DCddaIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DCddaIOFactory::createEnumerator, this));
}

DCddaIOFactory::~DCddaIOFactory()
{
}

QSharedPointer<DFileInfo> DCddaIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DCddaIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DCddaIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DCddaIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DCddaIOFactory::createOperator() const
{
    return d->createOperator();
}
