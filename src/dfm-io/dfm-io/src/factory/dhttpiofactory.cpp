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

#include "factory/dhttpiofactory.h"
#include "factory/dhttpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DHttpIOFactoryPrivate::DHttpIOFactoryPrivate(DHttpIOFactory *q)
    : q_ptr(q)
{
}

DHttpIOFactoryPrivate::~DHttpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DHttpIOFactoryPrivate::createFileInfo() const
{
    Q_Q(const DHttpIOFactory);
    const QUrl &uri = q->uri();
    const QString &url = uri.url();

    return DLocalHelper::getFileInfo(url);
}

QSharedPointer<DFile> DHttpIOFactoryPrivate::createFile() const
{
    Q_Q(const DHttpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DHttpIOFactoryPrivate::createEnumerator() const
{
    Q_Q(const DHttpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DHttpIOFactoryPrivate::createWatcher() const
{
    Q_Q(const DHttpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DHttpIOFactoryPrivate::createOperator() const
{
    Q_Q(const DHttpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DHttpIOFactory::DHttpIOFactory(const QUrl &uri)
    : DIOFactory(uri)
    , d_ptr(new DHttpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DHttpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DHttpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DHttpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DHttpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DHttpIOFactory::createEnumerator, this));
}

DHttpIOFactory::~DHttpIOFactory()
{
}

QSharedPointer<DFileInfo> DHttpIOFactory::createFileInfo() const
{
    Q_D(const DHttpIOFactory);
    return d->createFileInfo();
}

QSharedPointer<DFile> DHttpIOFactory::createFile() const
{
    Q_D(const DHttpIOFactory);
    return d->createFile();
}

QSharedPointer<DEnumerator> DHttpIOFactory::createEnumerator() const
{
    Q_D(const DHttpIOFactory);
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DHttpIOFactory::createWatcher() const
{
    Q_D(const DHttpIOFactory);
    return d->createWatcher();
}

QSharedPointer<DOperator> DHttpIOFactory::createOperator() const
{
    Q_D(const DHttpIOFactory);
    return d->createOperator();
}
