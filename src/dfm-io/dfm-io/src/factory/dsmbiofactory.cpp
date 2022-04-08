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

#include "factory/dsmbiofactory.h"
#include "factory/dsmbiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DSmbIOFactoryPrivate::DSmbIOFactoryPrivate(DSmbIOFactory *q)
    : q(q)
{
}

DSmbIOFactoryPrivate::~DSmbIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DSmbIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DSmbIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DSmbIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DSmbIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DSmbIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DSmbIOFactory::DSmbIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DSmbIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DSmbIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DSmbIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DSmbIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DSmbIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DSmbIOFactory::createEnumerator, this));
}

DSmbIOFactory::~DSmbIOFactory()
{
}

QSharedPointer<DFileInfo> DSmbIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DSmbIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DSmbIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DSmbIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DSmbIOFactory::createOperator() const
{
    return d->createOperator();
}
