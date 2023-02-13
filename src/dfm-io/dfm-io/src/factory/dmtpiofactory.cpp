// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dmtpiofactory.h"
#include "factory/dmtpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DMtpIOFactoryPrivate::DMtpIOFactoryPrivate(DMtpIOFactory *q)
    : q(q)
{
}

DMtpIOFactoryPrivate::~DMtpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DMtpIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DMtpIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DMtpIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DMtpIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DMtpIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DMtpIOFactory::DMtpIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DMtpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DMtpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DMtpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DMtpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DMtpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DMtpIOFactory::createEnumerator, this));
}

DMtpIOFactory::~DMtpIOFactory()
{
}

QSharedPointer<DFileInfo> DMtpIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DMtpIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DMtpIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DMtpIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DMtpIOFactory::createOperator() const
{
    return d->createOperator();
}
