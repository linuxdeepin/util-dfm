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

#include "factory/dmtpiofactory.h"
#include "factory/dmtpiofactory_p.h"
#include "local/dlocalhelper.h"

#include "local/dlocalenumerator.h"
#include "local/dlocalwatcher.h"
#include "local/dlocaloperator.h"
#include "local/dlocalfile.h"

#include <QDebug>

USING_IO_NAMESPACE

DMtpIOFactoryPrivate::DMtpIOFactoryPrivate(DMtpIOFactory *q)
    : q(q)
{
}

DMtpIOFactoryPrivate::~DMtpIOFactoryPrivate()
{
}

QSharedPointer<DFileInfo> DMtpIOFactoryPrivate::createFileInfo() const
{
    const QUrl &uri = q->uri();
    return DLocalHelper::createFileInfoByUri(uri);
}

QSharedPointer<DFile> DMtpIOFactoryPrivate::createFile() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalFile>(new DLocalFile(uri));
}

QSharedPointer<DEnumerator> DMtpIOFactoryPrivate::createEnumerator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalEnumerator>(new DLocalEnumerator(uri));
}

QSharedPointer<DWatcher> DMtpIOFactoryPrivate::createWatcher() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalWatcher>(new DLocalWatcher(uri));
}

QSharedPointer<DOperator> DMtpIOFactoryPrivate::createOperator() const
{
    const QUrl &uri = q->uri();

    return QSharedPointer<DLocalOperator>(new DLocalOperator(uri));
}

DMtpIOFactory::DMtpIOFactory(const QUrl &uri)
    : DIOFactory(uri), d(new DMtpIOFactoryPrivate(this))
{
    registerCreateFileInfo(std::bind(&DMtpIOFactory::createFileInfo, this));
    registerCreateFile(std::bind(&DMtpIOFactory::createFile, this));
    registerCreateWatcher(std::bind(&DMtpIOFactory::createWatcher, this));
    registerCreateOperator(std::bind(&DMtpIOFactory::createOperator, this));
    registerCreateEnumerator(std::bind(&DMtpIOFactory::createEnumerator, this));
}

DMtpIOFactory::~DMtpIOFactory()
{
}

QSharedPointer<DFileInfo> DMtpIOFactory::createFileInfo() const
{
    return d->createFileInfo();
}

QSharedPointer<DFile> DMtpIOFactory::createFile() const
{
    return d->createFile();
}

QSharedPointer<DEnumerator> DMtpIOFactory::createEnumerator() const
{
    return d->createEnumerator();
}

QSharedPointer<DWatcher> DMtpIOFactory::createWatcher() const
{
    return d->createWatcher();
}

QSharedPointer<DOperator> DMtpIOFactory::createOperator() const
{
    return d->createOperator();
}
