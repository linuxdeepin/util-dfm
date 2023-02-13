// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dgphoto2iofactory.h"
#include "factory/dgphoto2iofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DGphoto2IOFactoryPrivate::DGphoto2IOFactoryPrivate(DGphoto2IOFactory *q)
    : q(q)
{
}

DGphoto2IOFactoryPrivate::~DGphoto2IOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DGphoto2IOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DGphoto2IOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DGphoto2IOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DGphoto2IOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DGphoto2IOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DGphoto2IOFactory::DGphoto2IOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DGphoto2IOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DGphoto2IOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DGphoto2IOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DGphoto2IOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DGphoto2IOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DGphoto2IOFactory::createEnumerator, this));
}

DGphoto2IOFactory::~DGphoto2IOFactory()
{
}

QSharedPointer<DFileInfo> DGphoto2IOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DGphoto2IOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DGphoto2IOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DGphoto2IOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DGphoto2IOFactory::createOperator() const
{
    return d->createOperator();
}
