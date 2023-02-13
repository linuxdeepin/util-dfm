// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
