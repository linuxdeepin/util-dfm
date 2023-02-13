// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dftpiofactory.h"
#include "factory/dftpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DFtpIOFactoryPrivate::DFtpIOFactoryPrivate(DFtpIOFactory *q)
    : q(q)
{
}

DFtpIOFactoryPrivate::~DFtpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DFtpIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DFtpIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DFtpIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DFtpIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DFtpIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DFtpIOFactory::DFtpIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DFtpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DFtpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DFtpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DFtpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DFtpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DFtpIOFactory::createEnumerator, this));
}

DFtpIOFactory::~DFtpIOFactory()
{
}

QSharedPointer<DFileInfo> DFtpIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DFtpIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DFtpIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DFtpIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DFtpIOFactory::createOperator() const
{
    return d->createOperator();
}
