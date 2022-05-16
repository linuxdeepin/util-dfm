/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
