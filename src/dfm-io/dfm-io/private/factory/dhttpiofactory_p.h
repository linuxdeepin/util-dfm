// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DHTTPIOFACTORY_P_H
#define DHTTPIOFACTORY_P_H

#include "dfmio_global.h"

#include "core/diofactory_p.h"
#include "core/dfileinfo.h"
#include "core/dfile.h"
#include "core/denumerator.h"
#include "core/dwatcher.h"
#include "core/doperator.h"

#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DHttpIOFactory;

class DHttpIOFactoryPrivate
{
public:
    explicit DHttpIOFactoryPrivate(DHttpIOFactory *q);
    ~DHttpIOFactoryPrivate();

    QSharedPointer<DFileInfo> createFileInfo() const;
    QSharedPointer<DFile> createFile() const;
    QSharedPointer<DEnumerator> createEnumerator() const;
    QSharedPointer<DWatcher> createWatcher() const;
    QSharedPointer<DOperator> createOperator() const;

public:
    DHttpIOFactory *q = nullptr;
};

END_IO_NAMESPACE

#endif // DHTTPIOFACTORY_P_H
