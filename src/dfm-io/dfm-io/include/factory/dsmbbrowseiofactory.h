// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSMBBROWSEIOFACTORY_H
#define DSMBBROWSEIOFACTORY_H

#include "dfmio_global.h"

#include "core/diofactory.h"

BEGIN_IO_NAMESPACE

class DSmbbrowseIOFactoryPrivate;

class DSmbbrowseIOFactory : public DIOFactory
{
public:
    explicit DSmbbrowseIOFactory(const QUrl &uri);
    ~DSmbbrowseIOFactory();

    QSharedPointer<DFileInfo> createFileInfo() const DFM_OVERRIDE;
    QSharedPointer<DFile> createFile() const DFM_OVERRIDE;
    QSharedPointer<DEnumerator> createEnumerator() const DFM_OVERRIDE;
    QSharedPointer<DWatcher> createWatcher() const DFM_OVERRIDE;
    QSharedPointer<DOperator> createOperator() const DFM_OVERRIDE;

private:
    QSharedPointer<DSmbbrowseIOFactoryPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif // DSMBBROWSEIOFACTORY_H
