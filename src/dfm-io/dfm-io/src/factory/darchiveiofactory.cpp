// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/darchiveiofactory.h"
#include "factory/darchiveiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DArchiveIOFactoryPrivate::DArchiveIOFactoryPrivate(DArchiveIOFactory *q)
    : q(q)
{
}

DArchiveIOFactoryPrivate::~DArchiveIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DArchiveIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DArchiveIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DArchiveIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DArchiveIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DArchiveIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DArchiveIOFactory::DArchiveIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DArchiveIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DArchiveIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DArchiveIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DArchiveIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DArchiveIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DArchiveIOFactory::createEnumerator, this));
}

DArchiveIOFactory::~DArchiveIOFactory()
{
}

QSharedPointer<DFileInfo> DArchiveIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DArchiveIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DArchiveIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DArchiveIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DArchiveIOFactory::createOperator() const
{
    return d->createOperator();
}
