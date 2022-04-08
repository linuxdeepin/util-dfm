/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "factory/darchiveiofactory.h"
#include "factory/darchiveiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DArchiveIOFactoryPrivate::DArchiveIOFactoryPrivate(DArchiveIOFactory *q)
    : q(q)
{
}

DArchiveIOFactoryPrivate::~DArchiveIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DArchiveIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DArchiveIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DArchiveIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DArchiveIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DArchiveIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DArchiveIOFactory::DArchiveIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DArchiveIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DArchiveIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DArchiveIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DArchiveIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DArchiveIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DArchiveIOFactory::createEnumerator, this));
}

DArchiveIOFactory::~DArchiveIOFactory()
{
}

QSharedPointer<DFileInfo> DArchiveIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DArchiveIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DArchiveIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DArchiveIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DArchiveIOFactory::createOperator() const
{
    return d->createOperator();
}
