// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

bool DEnumerator::init()
{
    if (d->initFunc)
        return d->initFunc();
    return false;
}

void DEnumerator::initAsync(int ioPriority, DEnumerator::InitCallbackFunc func, void *userData)
{
    if (d->initAsyncFunc)
        return d->initAsyncFunc(ioPriority, func, userData);
}

bool DEnumerator::cancel()
{
    if (!d->cancelFunc)
        return false;
    return d->cancelFunc();
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

QList<QSharedPointer<DFileInfo>> DEnumerator::fileInfoList()
{
    if (d->fileInfoListFunc)
        return d->fileInfoListFunc();
    return {};
}

void DEnumerator::registerFileInfoList(const DEnumerator::FileInfoListFunc &func)
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

void DEnumerator::registerInit(const DEnumerator::InitFunc &func)
{
    d->initFunc = func;
}

void DEnumerator::registerInitAsync(const DEnumerator::InitAsyncFunc &func)
{
    d->initAsyncFunc = func;
}

void DEnumerator::registerCancel(const DEnumerator::CancelFunc &func)
{
    d->cancelFunc = func;
}
