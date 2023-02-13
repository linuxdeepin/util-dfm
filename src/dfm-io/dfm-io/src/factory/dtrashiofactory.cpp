// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dtrashiofactory.h"
#include "factory/dtrashiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DTrashIOFactoryPrivate::DTrashIOFactoryPrivate(DTrashIOFactory *q)
    : q(q)
{
}

DTrashIOFactoryPrivate::~DTrashIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DTrashIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DTrashIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DTrashIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DTrashIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DTrashIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DTrashIOFactory::DTrashIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DTrashIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DTrashIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DTrashIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DTrashIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DTrashIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DTrashIOFactory::createEnumerator, this));
}

DTrashIOFactory::~DTrashIOFactory()
{
}

QSharedPointer<DFileInfo> DTrashIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DTrashIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DTrashIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DTrashIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DTrashIOFactory::createOperator() const
{
    return d->createOperator();
}
