// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DMONITORPRIVATE_H
#define DMONITORPRIVATE_H

#include <dfm-mount/base/dmount_global.h>
#include <dfm-mount/base/ddevicemonitor.h>

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
