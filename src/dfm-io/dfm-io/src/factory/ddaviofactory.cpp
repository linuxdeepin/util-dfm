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

#include "factory/ddaviofactory.h"
#include "factory/ddaviofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DDavIOFactoryPrivate::DDavIOFactoryPrivate(DDavIOFactory *q)
    : q_ptr(q)
{
}

DDavIOFactoryPrivate::~DDavIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DDavIOFactoryPrivate::createFileInfo() const
{
    Q_Q(const DDavIOFactory);
    const QUrl &uri = q->uri();
    const QString &url = uri.url();

    return DLocalHelper::getFileInfo(url);
}

QSharedPointer<DFile> DDavIOFactoryPrivate::createFile() const
{
    Q_Q(const DDavIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DDavIOFactoryPrivate::createEnumerator() const
{
    Q_Q(const DDavIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DDavIOFactoryPrivate::createWatcher() const
{
    Q_Q(const DDavIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DDavIOFactoryPrivate::createOperator() const
{
    Q_Q(const DDavIOFactory);
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DDavIOFactory::DDavIOFactory(const QUrl &uri)
    : DIOFactory(uri)
    , d_ptr(new DDavIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DDavIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DDavIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DDavIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DDavIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DDavIOFactory::createEnumerator, this));
}

DDavIOFactory::~DDavIOFactory()
{
}

QSharedPointer<DFileInfo> DDavIOFactory::createFileInfo() const
{
    Q_D(const DDavIOFactory);
    return d->createFileInfo();
}

QSharedPointer<DFile> DDavIOFactory::createFile() const
{
    Q_D(const DDavIOFactory);
    return d->createFile();
}

QSharedPointer<DEnumerator> DDavIOFactory::createEnumerator() const
{
    Q_D(const DDavIOFactory);
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DDavIOFactory::createWatcher() const
{
    Q_D(const DDavIOFactory);
    return d->createWatcher();
}

QSharedPointer<DOperator> DDavIOFactory::createOperator() const
{
    Q_D(const DDavIOFactory);
    return d->createOperator();
}
