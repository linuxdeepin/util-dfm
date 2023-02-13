// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dafpbrowseiofactory.h"
#include "factory/dafpbrowseiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DAfpbrowseIOFactoryPrivate::DAfpbrowseIOFactoryPrivate(DAfpbrowseIOFactory *q)
    : q(q)
{
}

DAfpbrowseIOFactoryPrivate::~DAfpbrowseIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DAfpbrowseIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DAfpbrowseIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DAfpbrowseIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DAfpbrowseIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DAfpbrowseIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DAfpbrowseIOFactory::DAfpbrowseIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DAfpbrowseIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DAfpbrowseIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DAfpbrowseIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DAfpbrowseIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DAfpbrowseIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DAfpbrowseIOFactory::createEnumerator, this));
}

DAfpbrowseIOFactory::~DAfpbrowseIOFactory()
{
}

QSharedPointer<DFileInfo> DAfpbrowseIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DAfpbrowseIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DAfpbrowseIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DAfpbrowseIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DAfpbrowseIOFactory::createOperator() const
{
    return d->createOperator();
}
