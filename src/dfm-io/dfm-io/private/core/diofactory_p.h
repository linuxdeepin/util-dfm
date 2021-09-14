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
#ifndef DIOFACTORY_P_H
#define DIOFACTORY_P_H

#include <QUrl>
#include <QSharedPointer>

#include "dfmio_global.h"
#include "core/dfile.h"
#include "core/dfileinfo.h"
#include "core/denumerator.h"
#include "core/dwatcher.h"
#include "core/doperator.h"
#include "error/error.h"
#include "core/diofactory.h"

BEGIN_IO_NAMESPACE

class DIOFactory;

class DIOFactoryPrivate
{
public:
    explicit DIOFactoryPrivate(DIOFactory *q);
    virtual ~DIOFactoryPrivate();

    /*virtual QSharedPointer<DFileInfo> doCreateFileInfo() const = 0;
    virtual QSharedPointer<DFile> doCreateFile() const = 0;
    virtual QSharedPointer<DEnumerator> doCreateEnumerator() const = 0;
    virtual QSharedPointer<DWatcher> doCreateWatcher() const = 0;
    virtual QSharedPointer<DOperator> doCreateOperator() const = 0;*/

public:
    DIOFactory *q = nullptr;
    QUrl uri;

    DFMIOError error;

    DIOFactory::CreateFileInfoFunc createFileInfoFunc = nullptr;
    DIOFactory::CreateFileFunc createFileFunc = nullptr;
    DIOFactory::CreateEnumeratorFunc createEnumeratorFunc = nullptr;
    DIOFactory::CreateWatcherFunc createWatcherFunc = nullptr;
    DIOFactory::CreateOperatorFunc createOperatorFunc = nullptr;
};

END_IO_NAMESPACE

#endif // DIOFACTORY_P_H
