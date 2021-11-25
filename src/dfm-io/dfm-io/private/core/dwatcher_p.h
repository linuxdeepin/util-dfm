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

#include "dfmio_global.h"

#include "core/dfileinfo.h"
#include "core/dwatcher.h"

#include <QUrl>

BEGIN_IO_NAMESPACE

class DWatcher;

class DWatcherPrivate
{
public:
    explicit DWatcherPrivate(DWatcher *q);
    virtual ~DWatcherPrivate();

public:
    DWatcher *q = nullptr;
    QUrl uri;

    DWatcher::RunningFunc runningFunc = nullptr;
    DWatcher::StartFunc startFunc = nullptr;
    DWatcher::StopFunc stopFunc = nullptr;
    DWatcher::SetWatchTypeFunc setWatchTypeFunc = nullptr;
    DWatcher::WatchTypeFunc watchTypeFunc = nullptr;
    DWatcher::LastErrorFunc lastErrorFunc = nullptr;

    QList<DFileInfo::AttributeID> ids;
    int timeRate = 200;
};

END_IO_NAMESPACE

#endif   // DWATCHER_P_H
