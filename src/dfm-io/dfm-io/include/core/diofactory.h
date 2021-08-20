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
#ifndef DIOFACTORY_H
#define DIOFACTORY_H

#include "dfmio_global.h"

#include "error/error.h"

#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DIOFactoryPrivate;
class DFileInfo;
class DFile;
class DEnumerator;
class DWatcher;
class DOperator;

class DIOFactory
{
public:
    DIOFactory() = default;

    void setUri(const QUrl &uri);
    QUrl uri() const;

    DFM_VIRTUAL QSharedPointer<DFileInfo> createFileInfo() const;
    DFM_VIRTUAL QSharedPointer<DFile> createFile() const;
    DFM_VIRTUAL QSharedPointer<DEnumerator> createEnumerator() const;
    DFM_VIRTUAL QSharedPointer<DWatcher> createWatcher() const;
    DFM_VIRTUAL QSharedPointer<DOperator> createOperator() const;

    DFMIOError lastError() const;

protected:
    QScopedPointer<DIOFactoryPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(DIOFactory)
    Q_DISABLE_COPY(DIOFactory)
};

END_IO_NAMESPACE


#endif // DIOFACTORY_H
