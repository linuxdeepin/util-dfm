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
#include "core/denumerator_p.h"
#include "core/denumerator.h"
#include "core/dfileinfo.h"

#include <QVariant>

USING_IO_NAMESPACE

DEnumeratorPrivate::DEnumeratorPrivate(DEnumerator *q)
    : q(q)
{
}

DEnumeratorPrivate::~DEnumeratorPrivate()
{
}

DEnumerator::DEnumerator(const QUrl &uri, const QStringList &nameFilters, DEnumerator::DirFilters filters, DEnumerator::IteratorFlags flags)
    : d(new DEnumeratorPrivate(this))
{
    d->uri = uri;
    d->nameFilters = nameFilters;
    d->dirFilters = filters;
    d->iteratorFlags = flags;
}

DEnumerator::~DEnumerator()
{
}

bool DEnumerator::hasNext() const
{
    if (d->hasNextFunc)
        return d->hasNextFunc();
    return false;
}

QUrl DEnumerator::next() const
{
    if (d->nextFunc)
        return d->nextFunc();
    return QUrl();
}

QUrl DEnumerator::uri() const
{
    return d->uri;
}

QStringList DEnumerator::nameFilters() const
{
    return d->nameFilters;
}

DEnumerator::DirFilters DEnumerator::dirFilters()
{
    return d->dirFilters;
}

DEnumerator::IteratorFlags DEnumerator::iteratorFlags() const
{
    return d->iteratorFlags;
}

void DEnumerator::setTimeout(ulong timeout)
{
    d->enumTimeout = timeout;
}

ulong DEnumerator::timeout() const
{
    return d->enumTimeout;
}

QSharedPointer<DFileInfo> DEnumerator::fileInfo() const
{
    if (d->fileInfoFunc)
        return d->fileInfoFunc();
    return nullptr;
}

quint64 DEnumerator::fileCount()
{
    if (d->fileCountFunc)
        return d->fileCountFunc();
    return 0;
}

void DEnumerator::registerFileInfoList(const FileInfoListFunc &func)
{
    d->fileInfoListFunc = func;
}

void DEnumerator::registerHasNext(const DEnumerator::HasNextFunc &func)
{
    d->hasNextFunc = func;
}

void DEnumerator::registerNext(const DEnumerator::NextFunc &func)
{
    d->nextFunc = func;
}

void DEnumerator::registerFileInfo(const DEnumerator::FileInfoFunc &func)
{
    d->fileInfoFunc = func;
}

void DEnumerator::registerFileCount(const DEnumerator::FileCountFunc &func)
{
    d->fileCountFunc = func;
}

void DEnumerator::registerLastError(const DEnumerator::LastErrorFunc &func)
{
    d->lastErrorFunc = func;
}

DFMIOError DEnumerator::lastError() const
{
    if (!d->lastErrorFunc)
        return DFMIOError();

    return d->lastErrorFunc();
}
