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

#include "factory/dnfsiofactory.h"
#include "factory/dnfsiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DNfsIOFactoryPrivate::DNfsIOFactoryPrivate(DNfsIOFactory *q)
    : q_ptr(q)
{
}

DNfsIOFactoryPrivate::~DNfsIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DNfsIOFactoryPrivate::createFileInfo() const
{
    Q_Q(const DNfsIOFactory);
    const QUrl &uri = q->uri();
    const QString &url = uri.url();

    return DLocalHelper::getFileInfo(url);
}

QSharedPointer<DFile> DNfsIOFactoryPrivate::createFile() const
{
    Q_Q(const DNfsIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DNfsIOFactoryPrivate::createEnumerator() const
{
    Q_Q(const DNfsIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DNfsIOFactoryPrivate::createWatcher() const
{
    Q_Q(const DNfsIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DNfsIOFactoryPrivate::createOperator() const
{
    Q_Q(const DNfsIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DNfsIOFactory::DNfsIOFactory(const QUrl &uri)
    : DIOFactory(uri)
    , d_ptr(new DNfsIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DNfsIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DNfsIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DNfsIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DNfsIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DNfsIOFactory::createEnumerator, this));
}

DNfsIOFactory::~DNfsIOFactory()
{
}

QSharedPointer<DFileInfo> DNfsIOFactory::createFileInfo() const
{
    Q_D(const DNfsIOFactory);
    return d->createFileInfo();
}

QSharedPointer<DFile> DNfsIOFactory::createFile() const
{
    Q_D(const DNfsIOFactory);
    return d->createFile();
}

QSharedPointer<DEnumerator> DNfsIOFactory::createEnumerator() const
{
    Q_D(const DNfsIOFactory);
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DNfsIOFactory::createWatcher() const
{
    Q_D(const DNfsIOFactory);
    return d->createWatcher();
}

QSharedPointer<DOperator> DNfsIOFactory::createOperator() const
{
    Q_D(const DNfsIOFactory);
    return d->createOperator();
}
