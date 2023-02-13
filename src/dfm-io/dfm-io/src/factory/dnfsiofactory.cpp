// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dnfsiofactory.h"
#include "factory/dnfsiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DNfsIOFactoryPrivate::DNfsIOFactoryPrivate(DNfsIOFactory *q)
    : q(q)
{
}

DNfsIOFactoryPrivate::~DNfsIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DNfsIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DNfsIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DNfsIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DNfsIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DNfsIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DNfsIOFactory::DNfsIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DNfsIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DNfsIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DNfsIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DNfsIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DNfsIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DNfsIOFactory::createEnumerator, this));
}

DNfsIOFactory::~DNfsIOFactory()
{
}

QSharedPointer<DFileInfo> DNfsIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DNfsIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DNfsIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DNfsIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DNfsIOFactory::createOperator() const
{
    return d->createOperator();
}
