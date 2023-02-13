// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dsmbbrowseiofactory.h"
#include "factory/dsmbbrowseiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DSmbbrowseIOFactoryPrivate::DSmbbrowseIOFactoryPrivate(DSmbbrowseIOFactory *q)
    : q(q)
{
}

DSmbbrowseIOFactoryPrivate::~DSmbbrowseIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DSmbbrowseIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DSmbbrowseIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DSmbbrowseIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DSmbbrowseIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DSmbbrowseIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DSmbbrowseIOFactory::DSmbbrowseIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DSmbbrowseIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DSmbbrowseIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DSmbbrowseIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DSmbbrowseIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DSmbbrowseIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DSmbbrowseIOFactory::createEnumerator, this));
}

DSmbbrowseIOFactory::~DSmbbrowseIOFactory()
{
}

QSharedPointer<DFileInfo> DSmbbrowseIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DSmbbrowseIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DSmbbrowseIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DSmbbrowseIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DSmbbrowseIOFactory::createOperator() const
{
    return d->createOperator();
}
