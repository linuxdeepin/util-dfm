// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/ddnssdiofactory.h"
#include "factory/ddnssdiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DDnssdIOFactoryPrivate::DDnssdIOFactoryPrivate(DDnssdIOFactory *q)
    : q(q)
{
}

DDnssdIOFactoryPrivate::~DDnssdIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DDnssdIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DDnssdIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DDnssdIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DDnssdIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DDnssdIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DDnssdIOFactory::DDnssdIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DDnssdIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DDnssdIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DDnssdIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DDnssdIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DDnssdIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DDnssdIOFactory::createEnumerator, this));
}

DDnssdIOFactory::~DDnssdIOFactory()
{
}

QSharedPointer<DFileInfo> DDnssdIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DDnssdIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DDnssdIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DDnssdIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DDnssdIOFactory::createOperator() const
{
    return d->createOperator();
}
