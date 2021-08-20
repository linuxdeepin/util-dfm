/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#include "core/diofactory.h"
#include "error.h"
#include "core/diofactory_p.h"

USING_IO_NAMESPACE

DIOFactoryPrivate::DIOFactoryPrivate(DIOFactory *q)
    : q_ptr(q)
{

}

DIOFactoryPrivate::~DIOFactoryPrivate()
{

}

void DIOFactory::setUri(const QUrl &uri)
{
    Q_D(DIOFactory);

    if (!d)
        return;

    d->uri = uri;
}

QUrl DIOFactory::uri() const
{
    Q_D(const DIOFactory);

    if (!d)
        return QUrl();

    return d->uri;
}

DFM_VIRTUAL QSharedPointer<DFileInfo> DIOFactory::createFileInfo() const
{
    Q_D(const DIOFactory);

    if (!d)
        return nullptr;

    return d->doCreateFileInfo();
}

DFM_VIRTUAL QSharedPointer<DFile> DIOFactory::createFile() const
{
    Q_D(const DIOFactory);

    if (!d)
        return nullptr;

    return d->doCreateFile();
}

DFM_VIRTUAL QSharedPointer<DEnumerator> DIOFactory::createEnumerator() const
{
    Q_D(const DIOFactory);

    if (!d)
        return nullptr;

    return d->doCreateEnumerator();
}

DFM_VIRTUAL QSharedPointer<DWatcher> DIOFactory::createWatcher() const
{
    Q_D(const DIOFactory);

    if (!d)
        return nullptr;

    return d->doCreateWatcher();
}

DFM_VIRTUAL QSharedPointer<DOperator> DIOFactory::createOperator() const
{
    Q_D(const DIOFactory);

    if (!d)
        return nullptr;

    return d->doCreateOperator();
}

DFMIOError DIOFactory::lastError() const
{
    Q_D(const DIOFactory);

    if (!d)
        return DFMIOError();

    return d->error;
}
