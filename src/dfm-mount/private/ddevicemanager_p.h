// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDEVICEMANAGERPRIVATE_H
#define DDEVICEMANAGERPRIVATE_H

#include <dfm-mount/base/dmount_global.h>
#include <dfm-mount/ddevicemanager.h>
#include <dfm-mount/dblockmonitor.h>
#include <dfm-mount/dprotocolmonitor.h>

#include <QMap>

DFM_MOUNT_BEGIN_NS

class DDeviceManagerPrivate final
{
public:
    DDeviceManagerPrivate(DDeviceManager *qq);

    template<typename DFMSubMonitor, typename... ConstructArgs>
    bool registerMonitor(ConstructArgs &&... args);
    QSharedPointer<DDeviceMonitor> getRegisteredMonitor(DeviceType type) const;
    bool startMonitor();
    bool stopMonitor();
    QMap<DeviceType, QStringList> devices(DeviceType type);

    QMap<DeviceType, QSharedPointer<DDeviceMonitor>> monitors;
    MonitorError lastError;

    DDeviceManager *q = nullptr;
};

DFM_MOUNT_END_NS

#endif   // DDEVICEMANAGERPRIVATE_H
