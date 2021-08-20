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

#ifndef DENUMERATOR_H
#define DENUMERATOR_H

#include "dfmio_global.h"

#include "error/error.h"

#include <QSharedPointer>
#include <QUrl>

#include <functional>

BEGIN_IO_NAMESPACE

class DEnumeratorPrivate;
class DFileInfo;

class DEnumerator
{
public:
    using FileInfoListFunc = std::function<QList<QSharedPointer<DFileInfo>>()>;

public:
    DEnumerator(const QUrl &uri);
    virtual ~DEnumerator();

    QUrl uri() const;

    bool hasNext() const;
    QString next();
    QSharedPointer<DFileInfo> fileInfo() const;

    // register
    void registerFileInfoList(const FileInfoListFunc &func);

    DFMIOError lastError() const;

private:
    QSharedPointer<DEnumeratorPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DEnumerator)
};

END_IO_NAMESPACE

#endif // DENUMERATOR_H
