// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dburniofactory.h"
#include "factory/dburniofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DBurnIOFactoryPrivate::DBurnIOFactoryPrivate(DBurnIOFactory *q)
    : q(q)
{
}

DBurnIOFactoryPrivate::~DBurnIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DBurnIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DBurnIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DBurnIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DBurnIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DBurnIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DBurnIOFactory::DBurnIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DBurnIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DBurnIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DBurnIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DBurnIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DBurnIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DBurnIOFactory::createEnumerator, this));
}

DBurnIOFactory::~DBurnIOFactory()
{
}

QSharedPointer<DFileInfo> DBurnIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DBurnIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DBurnIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DBurnIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DBurnIOFactory::createOperator() const
{
    return d->createOperator();
}
