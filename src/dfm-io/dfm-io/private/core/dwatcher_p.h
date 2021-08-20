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
#ifndef DWATCHER_P_H
#define DWATCHER_P_H

#include "error/error.h"
#include "dfmio_global.h"

#include "core/dfileinfo.h"
#include "core/dwatcher.h"

#include <QUrl>

BEGIN_IO_NAMESPACE

class DWatcher;

class DWatcherPrivate
{
    Q_DECLARE_PUBLIC(DWatcher)
public:
    explicit DWatcherPrivate(DWatcher *q);
    virtual ~DWatcherPrivate();

    /*virtual bool start(int timeRate) = 0;
    virtual bool stop() = 0;*/

public:
    DWatcher *q_ptr;
    QUrl uri;

    DWatcher::StartFunc startFunc = nullptr;
    DWatcher::StopFunc stopFunc = nullptr;

    QList<DFileInfo::AttributeID> ids;
    int timeRate = 0;
    DFMIOError error;
};

END_IO_NAMESPACE

#endif // DWATCHER_P_H
