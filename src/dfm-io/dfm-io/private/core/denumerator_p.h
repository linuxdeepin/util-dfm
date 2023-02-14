// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    DEnumerator::SetArgumentsFunc setArgumentsFunc = nullptr;
    DEnumerator::SortFileInfoListFunc sortFileInfoListFunc = nullptr;
    DEnumerator::InitFunc initFunc = nullptr;
    DEnumerator::InitAsyncFunc initAsyncFunc = nullptr;
    DEnumerator::CancelFunc cancelFunc = nullptr;

    QUrl uri;
    QStringList nameFilters;
    DEnumerator::DirFilters dirFilters = DEnumerator::DirFilter::kNoFilter;
    DEnumerator::IteratorFlags iteratorFlags = DEnumerator::IteratorFlag::kNoIteratorFlags;
    ulong enumTimeout { 0 };
};

END_IO_NAMESPACE

#endif   // DENUMERATOR_P_H
