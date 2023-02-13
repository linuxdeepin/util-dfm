// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dddaviofactory.h"
#include "factory/dddaviofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DDdavIOFactoryPrivate::DDdavIOFactoryPrivate(DDdavIOFactory *q)
    : q(q)
{
}

DDdavIOFactoryPrivate::~DDdavIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DDdavIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DDdavIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DDdavIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DDdavIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DDdavIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DDdavIOFactory::DDdavIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DDdavIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DDdavIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DDdavIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DDdavIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DDdavIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DDdavIOFactory::createEnumerator, this));
}

DDdavIOFactory::~DDdavIOFactory()
{
}

QSharedPointer<DFileInfo> DDdavIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DDdavIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DDdavIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DDdavIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DDdavIOFactory::createOperator() const
{
    return d->createOperator();
}
