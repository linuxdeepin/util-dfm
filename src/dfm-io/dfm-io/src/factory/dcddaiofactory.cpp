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

#include "factory/dcddaiofactory.h"
#include "factory/dcddaiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DCddaIOFactoryPrivate::DCddaIOFactoryPrivate(DCddaIOFactory *q)
    : q_ptr(q)
{
}

DCddaIOFactoryPrivate::~DCddaIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DCddaIOFactoryPrivate::createFileInfo() const
{
    Q_Q(const DCddaIOFactory);
    const QUrl &uri = q->uri();
    const QString &url = uri.url();

    return DLocalHelper::getFileInfo(url);
}

QSharedPointer<DFile> DCddaIOFactoryPrivate::createFile() const
{
    Q_Q(const DCddaIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DCddaIOFactoryPrivate::createEnumerator() const
{
    Q_Q(const DCddaIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DCddaIOFactoryPrivate::createWatcher() const
{
    Q_Q(const DCddaIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DCddaIOFactoryPrivate::createOperator() const
{
    Q_Q(const DCddaIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DCddaIOFactory::DCddaIOFactory(const QUrl &uri)
    : DIOFactory(uri)
    , d_ptr(new DCddaIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DCddaIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DCddaIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DCddaIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DCddaIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DCddaIOFactory::createEnumerator, this));
}

DCddaIOFactory::~DCddaIOFactory()
{
}

QSharedPointer<DFileInfo> DCddaIOFactory::createFileInfo() const
{
    Q_D(const DCddaIOFactory);
    return d->createFileInfo();
}

QSharedPointer<DFile> DCddaIOFactory::createFile() const
{
    Q_D(const DCddaIOFactory);
    return d->createFile();
}

QSharedPointer<DEnumerator> DCddaIOFactory::createEnumerator() const
{
    Q_D(const DCddaIOFactory);
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DCddaIOFactory::createWatcher() const
{
    Q_D(const DCddaIOFactory);
    return d->createWatcher();
}

QSharedPointer<DOperator> DCddaIOFactory::createOperator() const
{
    Q_D(const DCddaIOFactory);
    return d->createOperator();
}
