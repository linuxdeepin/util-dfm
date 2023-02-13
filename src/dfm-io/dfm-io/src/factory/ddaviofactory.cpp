// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/ddaviofactory.h"
#include "factory/ddaviofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DDavIOFactoryPrivate::DDavIOFactoryPrivate(DDavIOFactory *q)
    : q(q)
{
}

DDavIOFactoryPrivate::~DDavIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DDavIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DDavIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DDavIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DDavIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DDavIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DDavIOFactory::DDavIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DDavIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DDavIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DDavIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DDavIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DDavIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DDavIOFactory::createEnumerator, this));
}

DDavIOFactory::~DDavIOFactory()
{
}

QSharedPointer<DFileInfo> DDavIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DDavIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DDavIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DDavIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DDavIOFactory::createOperator() const
{
    return d->createOperator();
}
