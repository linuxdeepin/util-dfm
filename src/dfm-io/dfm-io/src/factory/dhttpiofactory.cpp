// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dhttpiofactory.h"
#include "factory/dhttpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DHttpIOFactoryPrivate::DHttpIOFactoryPrivate(DHttpIOFactory *q)
    : q(q)
{
}

DHttpIOFactoryPrivate::~DHttpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DHttpIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DHttpIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DHttpIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DHttpIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DHttpIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DHttpIOFactory::DHttpIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DHttpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DHttpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DHttpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DHttpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DHttpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DHttpIOFactory::createEnumerator, this));
}

DHttpIOFactory::~DHttpIOFactory()
{
}

QSharedPointer<DFileInfo> DHttpIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DHttpIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DHttpIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DHttpIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DHttpIOFactory::createOperator() const
{
    return d->createOperator();
}
