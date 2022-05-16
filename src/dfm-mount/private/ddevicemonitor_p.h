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
#ifndef DMONITORPRIVATE_H
#define DMONITORPRIVATE_H

#include "base/dmount_global.h"
#include "base/ddevicemonitor.h"

#include <QMap>

#include <functional>

using namespace std;
DFM_MOUNT_BEGIN_NS

class DDeviceMonitorPrivate
{
public:
    DDeviceMonitorPrivate(DDeviceMonitor *qq);
    virtual ~DDeviceMonitorPrivate();

    DDeviceMonitor::StartMonitorFunc start = nullptr;
    DDeviceMonitor::StopMonitorFunc stop = nullptr;
    DDeviceMonitor::MonitorObjectTypeFunc monitorObjectType = nullptr;
    DDeviceMonitor::GetDevicesFunc getDevices = nullptr;
    DDeviceMonitor::CreateDeviceByIdFunc createDeviceById = nullptr;

    DDeviceMonitor *q = nullptr;
    // for saving gsignals connections, key: singal_name, value: the handler returned by g_signal_connect
    QMap<QString, ulong> connections;
    MonitorStatus monitorStatus = MonitorStatus::kIdle;
};

DFM_MOUNT_END_NS

#endif   // DMONITORPRIVATE_H
