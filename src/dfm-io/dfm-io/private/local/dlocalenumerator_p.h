/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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

#ifndef DLOCALENUMERATOR_P_H
#define DLOCALENUMERATOR_P_H

#include "dfmio_global.h"
#include "error/error.h"
#include "core/denumerator_p.h"

#include <gio/gio.h>

#include <QList>
#include <QMap>
#include <QSet>
#include <QStack>
#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DLocalEnumerator;
class DFileInfo;

class DLocalEnumeratorPrivate
{
public:
    explicit DLocalEnumeratorPrivate(DLocalEnumerator *q);
    ~DLocalEnumeratorPrivate();

    void init();

    QList<QSharedPointer<DFileInfo>> fileInfoList();
    bool hasNext();
    QString next() const;
    QSharedPointer<DFileInfo> fileInfo() const;
    bool checkFilter();

    DFMIOError lastError();
    void setErrorInfo(GError *gerror);
    void clean();

public:
    QList<QSharedPointer<DFileInfo>> list_;
    DLocalEnumerator *q = nullptr;
    QStack<GFileEnumerator *> stackEnumerator;
    QSharedPointer<DFileInfo> dfileInfoNext = nullptr;
    GFile *gfileNext = nullptr;
    bool enumSubDir = false;
    bool enumLinks = false;

    QMap<QString, QSet<QString>> hiddenFiles;
    QStringList nameFilters;
    DEnumerator::DirFilters dirFilters = DEnumerator::DirFilter::NoFilter;
    DEnumerator::IteratorFlags iteratorFlags = DEnumerator::IteratorFlag::NoIteratorFlags;

    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DLOCALENUMERATOR_P_H
