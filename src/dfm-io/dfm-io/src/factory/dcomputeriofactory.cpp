// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dcomputeriofactory.h"
#include "factory/dcomputeriofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DComputerIOFactoryPrivate::DComputerIOFactoryPrivate(DComputerIOFactory *q)
    : q(q)
{
}

DComputerIOFactoryPrivate::~DComputerIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DComputerIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DComputerIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DComputerIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DComputerIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DComputerIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DComputerIOFactory::DComputerIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DComputerIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DComputerIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DComputerIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DComputerIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DComputerIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DComputerIOFactory::createEnumerator, this));
}

DComputerIOFactory::~DComputerIOFactory()
{
}

QSharedPointer<DFileInfo> DComputerIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DComputerIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DComputerIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DComputerIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DComputerIOFactory::createOperator() const
{
    return d->createOperator();
}
