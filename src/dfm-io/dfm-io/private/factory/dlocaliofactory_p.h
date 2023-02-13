// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DLOCALIOFACTORY_P_H
#define DLOCALIOFACTORY_P_H

#include "dfmio_global.h"

#include "core/diofactory_p.h"
#include "core/dfileinfo.h"
#include "core/dfile.h"
#include "core/denumerator.h"
#include "core/dwatcher.h"
#include "core/doperator.h"

#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DLocalIOFactory;

class DLocalIOFactoryPrivate
{
public:
    explicit DLocalIOFactoryPrivate(DLocalIOFactory *q);
    ~DLocalIOFactoryPrivate();

    QSharedPointer<DFileInfo> createFileInfo(const char *attributes = "*",
                                             const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone) const;
    QSharedPointer<DFile> createFile() const;
    QSharedPointer<DEnumerator> createEnumerator(const QStringList &nameFilters = QStringList(), DEnumerator::DirFilters filters = DEnumerator::DirFilter::kNoFilter, DEnumerator::IteratorFlags flags = DEnumerator::IteratorFlag::kNoIteratorFlags) const;
    QSharedPointer<DWatcher> createWatcher() const;
    QSharedPointer<DOperator> createOperator() const;

public:
    DLocalIOFactory *q = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALIOFACTORY_P_H
