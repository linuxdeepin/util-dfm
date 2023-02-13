// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DLOCALIOFACTORY_H
#define DLOCALIOFACTORY_H

#include "dfmio_global.h"

#include "core/diofactory.h"

BEGIN_IO_NAMESPACE

class DLocalIOFactoryPrivate;

class DLocalIOFactory : public DIOFactory
{
public:
    explicit DLocalIOFactory(const QUrl &uri);
    ~DLocalIOFactory();

    QSharedPointer<DFile> createFile() const DFM_OVERRIDE;
    QSharedPointer<DFileInfo> createFileInfo(const char *attributes = "*",
                                             const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone) const DFM_OVERRIDE;
    QSharedPointer<DWatcher> createWatcher() const DFM_OVERRIDE;
    QSharedPointer<DOperator> createOperator() const DFM_OVERRIDE;
    QSharedPointer<DEnumerator> createEnumerator(const QStringList &nameFilters = QStringList(),
                                                 DEnumerator::DirFilters filters = DEnumerator::DirFilter::kNoFilter,
                                                 DEnumerator::IteratorFlags flags = DEnumerator::IteratorFlag::kNoIteratorFlags) const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalIOFactoryPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALIOFACTORY_H
