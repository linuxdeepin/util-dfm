// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDNSSDIOFACTORY_H
#define DDNSSDIOFACTORY_H

#include "dfmio_global.h"

#include "core/diofactory.h"

BEGIN_IO_NAMESPACE

class DDnssdIOFactoryPrivate;

class DDnssdIOFactory : public DIOFactory
{
public:
    explicit DDnssdIOFactory(const QUrl &uri);
    ~DDnssdIOFactory();

    QSharedPointer<DFileInfo> createFileInfo() const DFM_OVERRIDE;
    QSharedPointer<DFile> createFile() const DFM_OVERRIDE;
    QSharedPointer<DEnumerator> createEnumerator() const DFM_OVERRIDE;
    QSharedPointer<DWatcher> createWatcher() const DFM_OVERRIDE;
    QSharedPointer<DOperator> createOperator() const DFM_OVERRIDE;

private:
    QSharedPointer<DDnssdIOFactoryPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif // DDNSSDIOFACTORY_H
