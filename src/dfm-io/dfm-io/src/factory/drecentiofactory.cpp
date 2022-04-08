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

#include "factory/drecentiofactory.h"
#include "factory/drecentiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DRecentIOFactoryPrivate::DRecentIOFactoryPrivate(DRecentIOFactory *q)
    : q(q)
{
}

DRecentIOFactoryPrivate::~DRecentIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DRecentIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DRecentIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DRecentIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DRecentIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DRecentIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DRecentIOFactory::DRecentIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DRecentIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DRecentIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DRecentIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DRecentIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DRecentIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DRecentIOFactory::createEnumerator, this));
}

DRecentIOFactory::~DRecentIOFactory()
{
}

QSharedPointer<DFileInfo> DRecentIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DRecentIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DRecentIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DRecentIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DRecentIOFactory::createOperator() const
{
    return d->createOperator();
}
