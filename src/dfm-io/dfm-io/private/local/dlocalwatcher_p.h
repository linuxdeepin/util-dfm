/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#ifndef DLOCALWATCHER_P_H
#define DLOCALWATCHER_P_H

#include "dfmio_global.h"

#include "core/dwatcher_p.h"

#include "gio/gio.h"

BEGIN_IO_NAMESPACE

class DLocalWatcher;

class DLocalWatcherPrivate
{
    Q_DECLARE_PUBLIC(DLocalWatcher)
public:
    explicit DLocalWatcherPrivate(DLocalWatcher *q);
    ~DLocalWatcherPrivate();

    bool start(int timeRate);
    bool stop();

    static void watchCallback(GFileMonitor *monitor, GFile *child, GFile *other, GFileMonitorEvent event_type, gpointer user_data);

public:
    GFileMonitor *monitor = nullptr;

    DLocalWatcher *q_ptr;
};

END_IO_NAMESPACE

#endif // DLOCALWATCHER_P_H
