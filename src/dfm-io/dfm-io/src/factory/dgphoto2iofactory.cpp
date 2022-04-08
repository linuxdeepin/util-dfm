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
