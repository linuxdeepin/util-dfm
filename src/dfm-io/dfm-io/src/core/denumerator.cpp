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
    : q(q),
      done(false),
      info(nullptr)
{

}

DEnumeratorPrivate::~DEnumeratorPrivate()
{

}

DFM_VIRTUAL bool DEnumeratorPrivate::fetchMore()
{
    if (done)
        return false;

    if (!fileInfoListFunc)
        return false;

    const QList<QSharedPointer<DFileInfo>> &list = fileInfoListFunc();
    if (list.empty())
        return false;

    cached = std::move(list);

    done = true;
    return true;
}

QSharedPointer<DFileInfo> DEnumeratorPrivate::popCache()
{
    if (cached.empty())
        return nullptr;
    auto ret = cached.front();
    cached.pop_front();

    return ret;
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
    if (!d->cached.empty())
        return true;

    return d->fetchMore();
}

QString DEnumerator::next()
{
    if (!d)
        return "";

    auto fileInfo = d->popCache();
    if (!fileInfo) {
        if (d->fetchMore())
            fileInfo = d->popCache();
    }
    if (fileInfo != d->info)
        d->info = fileInfo;
    bool success;
    return fileInfo ? fileInfo->attribute(DFileInfo::AttributeID::StandardDisplayName, &success).toString() : QString();
}

QUrl DEnumerator::uri() const
{
    if (!d)
        return QUrl();

    return d->uri;
}

QSharedPointer<DFileInfo> DEnumerator::fileInfo() const
{
    if (!d)
        return nullptr;

    return d->info;
}

void DEnumerator::registerFileInfoList(const FileInfoListFunc &func)
{
    d->fileInfoListFunc = func;
}

DFMIOError DEnumerator::lastError() const
{
    if (!d)
        return DFMIOError();

    return d->dfmError;
}
