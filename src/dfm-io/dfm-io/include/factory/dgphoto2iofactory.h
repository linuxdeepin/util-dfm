// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DGPHOTO2IOFACTORY_H
#define DGPHOTO2IOFACTORY_H

#include "dfmio_global.h"

#include "core/diofactory.h"

BEGIN_IO_NAMESPACE

class DGphoto2IOFactoryPrivate;

class DGphoto2IOFactory : public DIOFactory
{
public:
    explicit DGphoto2IOFactory(const QUrl &uri);
    ~DGphoto2IOFactory();

    QSharedPointer<DFileInfo> createFileInfo() const DFM_OVERRIDE;
    QSharedPointer<DFile> createFile() const DFM_OVERRIDE;
    QSharedPointer<DEnumerator> createEnumerator() const DFM_OVERRIDE;
    QSharedPointer<DWatcher> createWatcher() const DFM_OVERRIDE;
    QSharedPointer<DOperator> createOperator() const DFM_OVERRIDE;

private:
    QSharedPointer<DGphoto2IOFactoryPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif // DGPHOTO2IOFACTORY_H
