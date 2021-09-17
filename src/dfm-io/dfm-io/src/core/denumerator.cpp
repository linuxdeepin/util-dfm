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

DEnumerator::DEnumerator(const QUrl &uri)
    : d(new DEnumeratorPrivate(this))
{
    d->uri = uri;
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

QString DEnumerator::next() const
{
    if (d->nextFunc)
        return d->nextFunc();
    return QString();
}

QUrl DEnumerator::uri() const
{
    if (!d)
        return QUrl();

    return d->uri;
}

QSharedPointer<DFileInfo> DEnumerator::fileInfo() const
{
    if (d->fileInfoFunc)
        return d->fileInfoFunc();
    return nullptr;
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

DFMIOError DEnumerator::lastError() const
{
    if (!d)
        return DFMIOError();

    return d->dfmError;
}
