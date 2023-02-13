// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DAFPBROWSEIOFACTORY_P_H
#define DAFPBROWSEIOFACTORY_P_H

#include "dfmio_global.h"

#include "core/diofactory_p.h"
#include "core/dfileinfo.h"
#include "core/dfile.h"
#include "core/denumerator.h"
#include "core/dwatcher.h"
#include "core/doperator.h"

#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DAfpbrowseIOFactory;

class DAfpbrowseIOFactoryPrivate
{
public:
    explicit DAfpbrowseIOFactoryPrivate(DAfpbrowseIOFactory *q);
    ~DAfpbrowseIOFactoryPrivate();

    QSharedPointer<DFileInfo> createFileInfo() const;
    QSharedPointer<DFile> createFile() const;
    QSharedPointer<DEnumerator> createEnumerator() const;
    QSharedPointer<DWatcher> createWatcher() const;
    QSharedPointer<DOperator> createOperator() const;

public:
    DAfpbrowseIOFactory *q = nullptr;
};

END_IO_NAMESPACE

#endif // DAFPBROWSEIOFACTORY_P_H
