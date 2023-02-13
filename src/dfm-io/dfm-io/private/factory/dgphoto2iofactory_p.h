// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DGPHOTO2IOFACTORY_P_H
#define DGPHOTO2IOFACTORY_P_H

#include "dfmio_global.h"

#include "core/diofactory_p.h"
#include "core/dfileinfo.h"
#include "core/dfile.h"
#include "core/denumerator.h"
#include "core/dwatcher.h"
#include "core/doperator.h"

#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DGphoto2IOFactory;

class DGphoto2IOFactoryPrivate
{
public:
    explicit DGphoto2IOFactoryPrivate(DGphoto2IOFactory *q);
    ~DGphoto2IOFactoryPrivate();

    QSharedPointer<DFileInfo> createFileInfo() const;
    QSharedPointer<DFile> createFile() const;
    QSharedPointer<DEnumerator> createEnumerator() const;
    QSharedPointer<DWatcher> createWatcher() const;
    QSharedPointer<DOperator> createOperator() const;

public:
    DGphoto2IOFactory *q = nullptr;
};

END_IO_NAMESPACE

#endif // DGPHOTO2IOFACTORY_P_H
