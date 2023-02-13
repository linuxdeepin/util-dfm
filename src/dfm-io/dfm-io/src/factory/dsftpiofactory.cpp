// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dsftpiofactory.h"
#include "factory/dsftpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DSftpIOFactoryPrivate::DSftpIOFactoryPrivate(DSftpIOFactory *q)
    : q(q)
{
}

DSftpIOFactoryPrivate::~DSftpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DSftpIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DSftpIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DSftpIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DSftpIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DSftpIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DSftpIOFactory::DSftpIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DSftpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DSftpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DSftpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DSftpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DSftpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DSftpIOFactory::createEnumerator, this));
}

DSftpIOFactory::~DSftpIOFactory()
{
}

QSharedPointer<DFileInfo> DSftpIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DSftpIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DSftpIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DSftpIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DSftpIOFactory::createOperator() const
{
    return d->createOperator();
}
