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

#include "factory/dcomputeriofactory.h"
#include "factory/dcomputeriofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DComputerIOFactoryPrivate::DComputerIOFactoryPrivate(DComputerIOFactory *q)
    : q(q)
{
}

DComputerIOFactoryPrivate::~DComputerIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DComputerIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DComputerIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DComputerIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DComputerIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DComputerIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DComputerIOFactory::DComputerIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DComputerIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DComputerIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DComputerIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DComputerIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DComputerIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DComputerIOFactory::createEnumerator, this));
}

DComputerIOFactory::~DComputerIOFactory()
{
}

QSharedPointer<DFileInfo> DComputerIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DComputerIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DComputerIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DComputerIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DComputerIOFactory::createOperator() const
{
    return d->createOperator();
}
