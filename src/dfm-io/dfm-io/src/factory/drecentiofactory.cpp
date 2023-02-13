// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/drecentiofactory.h"
#include "factory/drecentiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DRecentIOFactoryPrivate::DRecentIOFactoryPrivate(DRecentIOFactory *q)
    : q(q)
{
}

DRecentIOFactoryPrivate::~DRecentIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DRecentIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DRecentIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DRecentIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DRecentIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DRecentIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DRecentIOFactory::DRecentIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DRecentIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DRecentIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DRecentIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DRecentIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DRecentIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DRecentIOFactory::createEnumerator, this));
}

DRecentIOFactory::~DRecentIOFactory()
{
}

QSharedPointer<DFileInfo> DRecentIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DRecentIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DRecentIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DRecentIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DRecentIOFactory::createOperator() const
{
    return d->createOperator();
}
