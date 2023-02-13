// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DCDDAIOFACTORY_H
#define DCDDAIOFACTORY_H

#include "dfmio_global.h"

#include "core/diofactory.h"

BEGIN_IO_NAMESPACE

class DCddaIOFactoryPrivate;

class DCddaIOFactory : public DIOFactory
{
public:
    explicit DCddaIOFactory(const QUrl &uri);
    ~DCddaIOFactory();

    QSharedPointer<DFileInfo> createFileInfo() const DFM_OVERRIDE;
    QSharedPointer<DFile> createFile() const DFM_OVERRIDE;
    QSharedPointer<DEnumerator> createEnumerator() const DFM_OVERRIDE;
    QSharedPointer<DWatcher> createWatcher() const DFM_OVERRIDE;
    QSharedPointer<DOperator> createOperator() const DFM_OVERRIDE;

private:
    QSharedPointer<DCddaIOFactoryPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif // DCDDAIOFACTORY_H
