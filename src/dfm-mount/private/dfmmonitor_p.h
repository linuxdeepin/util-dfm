/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     xushitong<xushitong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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
#ifndef DFMMONITORPRIVATE_H
#define DFMMONITORPRIVATE_H

#include "base/dfmmount_global.h"
#include "base/dfmmonitor.h"

#include <QMap>

#include <functional>

using namespace std;
DFM_MOUNT_BEGIN_NS

class DFMMonitorPrivate
{
public:
    DFMMonitorPrivate(DFMMonitor *qq);
    virtual ~DFMMonitorPrivate();

    DFMMonitor::StartMonitorFunc start = nullptr;
    DFMMonitor::StopMonitorFunc stop = nullptr;
    DFMMonitor::MonitorObjectTypeFunc monitorObjectType = nullptr;
    DFMMonitor::GetDevicesFunc getDevices = nullptr;
    DFMMonitor::CreateDeviceByIdFunc createDeviceById = nullptr;

    DFMMonitor *q = nullptr;
    // for saving gsignals connections, key: singal_name, value: the handler returned by g_signal_connect
    QMap<QString, ulong> connections;
    MonitorStatus monitorStatus = MonitorStatus::Idle;
};

DFM_MOUNT_END_NS

#endif   // DFMMONITORPRIVATE_H
