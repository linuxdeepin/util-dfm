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

#include "factory/dsftpiofactory.h"
#include "factory/dsftpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DSftpIOFactoryPrivate::DSftpIOFactoryPrivate(DSftpIOFactory *q)
    : q_ptr(q)
{
}

DSftpIOFactoryPrivate::~DSftpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DSftpIOFactoryPrivate::createFileInfo() const
{
    Q_Q(const DSftpIOFactory);
    const QUrl &uri = q->uri();
    const QString &url = uri.url();

    return DLocalHelper::getFileInfo(url);
}

QSharedPointer<DFile> DSftpIOFactoryPrivate::createFile() const
{
    Q_Q(const DSftpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DSftpIOFactoryPrivate::createEnumerator() const
{
    Q_Q(const DSftpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DSftpIOFactoryPrivate::createWatcher() const
{
    Q_Q(const DSftpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DSftpIOFactoryPrivate::createOperator() const
{
    Q_Q(const DSftpIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DSftpIOFactory::DSftpIOFactory(const QUrl &uri)
    : DIOFactory(uri)
    , d_ptr(new DSftpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DSftpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DSftpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DSftpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DSftpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DSftpIOFactory::createEnumerator, this));
}

DSftpIOFactory::~DSftpIOFactory()
{
}

QSharedPointer<DFileInfo> DSftpIOFactory::createFileInfo() const
{
    Q_D(const DSftpIOFactory);
    return d->createFileInfo();
}

QSharedPointer<DFile> DSftpIOFactory::createFile() const
{
    Q_D(const DSftpIOFactory);
    return d->createFile();
}

QSharedPointer<DEnumerator> DSftpIOFactory::createEnumerator() const
{
    Q_D(const DSftpIOFactory);
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DSftpIOFactory::createWatcher() const
{
    Q_D(const DSftpIOFactory);
    return d->createWatcher();
}

QSharedPointer<DOperator> DSftpIOFactory::createOperator() const
{
    Q_D(const DSftpIOFactory);
    return d->createOperator();
}
