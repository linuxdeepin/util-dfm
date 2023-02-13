// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "factory/dlocaliofactory.h"
#include "factory/dlocaliofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DLocalIOFactoryPrivate::DLocalIOFactoryPrivate(DLocalIOFactory *q)
    : q(q)
{
}

DLocalIOFactoryPrivate::~DLocalIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DLocalIOFactoryPrivate::createFileInfo(const char *attributes /*= "*"*/,
                                                                 const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/) const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri, attributes, flag);
}

QSharedPointer<DFile> DLocalIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DLocalIOFactoryPrivate::createEnumerator(const QStringList &nameFilters, DEnumerator::DirFilters filters, DEnumerator::IteratorFlags flags) const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri, nameFilters, filters, flags));
}

QSharedPointer<DWatcher> DLocalIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DLocalIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DLocalIOFactory::DLocalIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DLocalIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DLocalIOFactory::createFileInfo, this, std::placeholders::_1, std::placeholders::_2));
    registerCreateFile(std::bind(&DLocalIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DLocalIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DLocalIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DLocalIOFactory::createEnumerator, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

DLocalIOFactory::~DLocalIOFactory()
{
}

QSharedPointer<DFileInfo> DLocalIOFactory::createFileInfo(const char *attributes /*= "*"*/,
                                                          const DFMIO::DFileInfo::FileQueryInfoFlags flag /*= DFMIO::DFileInfo::FileQueryInfoFlags::TypeNone*/) const
{
    return d->createFileInfo(attributes, flag);
}

QSharedPointer<DFile> DLocalIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DLocalIOFactory::createEnumerator(const QStringList &nameFilters, DEnumerator::DirFilters filters, DEnumerator::IteratorFlags flags) const
{
    return d->createEnumerator(nameFilters, filters, flags);
}

QSharedPointer<DWatcher> DLocalIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DLocalIOFactory::createOperator() const
{
    return d->createOperator();
}
