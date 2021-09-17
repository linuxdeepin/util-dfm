/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#ifndef DLOCALENUMERATOR_H
#define DLOCALENUMERATOR_H

#include "core/denumerator.h"

#include <QUrl>

BEGIN_IO_NAMESPACE

class DLocalEnumeratorPrivate;

class DLocalEnumerator : public DEnumerator
{
public:
    explicit DLocalEnumerator(const QUrl &uri);
    ~DLocalEnumerator();

    bool hasNext() const DFM_OVERRIDE;
    QString next() const DFM_OVERRIDE;
    QSharedPointer<DFileInfo> fileInfo() const DFM_OVERRIDE;

    QList<QSharedPointer<DFileInfo>> fileInfoList();

private:
    QSharedPointer<DLocalEnumeratorPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif // DLOCALENUMERATOR_H
