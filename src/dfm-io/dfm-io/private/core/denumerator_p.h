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
#ifndef DENUMERATOR_P_H
#define DENUMERATOR_P_H

#include "dfmio_global.h"

#include "core/denumerator.h"

#include <QSharedPointer>
#include <QUrl>

BEGIN_IO_NAMESPACE

class DEnumerator;
class DFileInfo;

class DEnumeratorPrivate
{
public:
    explicit DEnumeratorPrivate(DEnumerator *q);
    virtual ~DEnumeratorPrivate();

public:
    DEnumerator *q = nullptr;

    DEnumerator::HasNextFunc hasNextFunc = nullptr;
    DEnumerator::NextFunc nextFunc = nullptr;
    DEnumerator::FileInfoFunc fileInfoFunc = nullptr;
    DEnumerator::FileCountFunc fileCountFunc = nullptr;
    DEnumerator::LastErrorFunc lastErrorFunc = nullptr;
    DEnumerator::FileInfoListFunc fileInfoListFunc = nullptr;

    QUrl uri;
    QStringList nameFilters;
    DEnumerator::DirFilters dirFilters = DEnumerator::DirFilter::kNoFilter;
    DEnumerator::IteratorFlags iteratorFlags = DEnumerator::IteratorFlag::kNoIteratorFlags;
    ulong enumTimeout { 0 };
};

END_IO_NAMESPACE

#endif   // DENUMERATOR_P_H
