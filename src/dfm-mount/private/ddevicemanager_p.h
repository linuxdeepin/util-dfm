/*
 * Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
 *
 * Author:     xushitong <xushitong@uniontech.com>
 *
 * Maintainer: xushitong <xushitong@uniontech.com>
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

#ifndef DDEVICEMANAGERPRIVATE_H
#define DDEVICEMANAGERPRIVATE_H

#include "base/dmount_global.h"
#include "ddevicemanager.h"
#include "dblockmonitor.h"
#include "dprotocolmonitor.h"

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
