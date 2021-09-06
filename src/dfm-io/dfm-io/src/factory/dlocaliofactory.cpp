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
    : q_ptr(q)
{
}

DLocalIOFactoryPrivate::~DLocalIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DLocalIOFactoryPrivate::createFileInfo() const
{
    Q_Q(const DLocalIOFactory);
    const QUrl &uri = q->uri();
    const QString &url = uri.url();

    return DLocalHelper::getFileInfo(url);
}

QSharedPointer<DFile> DLocalIOFactoryPrivate::createFile() const
{
    Q_Q(const DLocalIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DLocalIOFactoryPrivate::createEnumerator() const
{
    Q_Q(const DLocalIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DLocalIOFactoryPrivate::createWatcher() const
{
    Q_Q(const DLocalIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DLocalIOFactoryPrivate::createOperator() const
{
    Q_Q(const DLocalIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DLocalIOFactory::DLocalIOFactory(const QUrl &uri)
    : DIOFactory(uri)
    , d_ptr(new DLocalIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DLocalIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DLocalIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DLocalIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DLocalIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DLocalIOFactory::createEnumerator, this));
}

DLocalIOFactory::~DLocalIOFactory()
{
}

QSharedPointer<DFileInfo> DLocalIOFactory::createFileInfo() const
{
    Q_D(const DLocalIOFactory);
    return d->createFileInfo();
}

QSharedPointer<DFile> DLocalIOFactory::createFile() const
{
    Q_D(const DLocalIOFactory);
    return d->createFile();
}

QSharedPointer<DEnumerator> DLocalIOFactory::createEnumerator() const
{
    Q_D(const DLocalIOFactory);
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DLocalIOFactory::createWatcher() const
{
    Q_D(const DLocalIOFactory);
    return d->createWatcher();
}

QSharedPointer<DOperator> DLocalIOFactory::createOperator() const
{
    Q_D(const DLocalIOFactory);
    return d->createOperator();
}
