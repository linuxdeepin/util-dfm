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

#ifndef DFMDEVICEMANAGERPRIVATE_H
#define DFMDEVICEMANAGERPRIVATE_H

#include "dfmdevicemanager.h"
#include "dfmblockmonitor.h"
#include "dfmprotocolmonitor.h"
#include "base/dfmmount_global.h"

#include <QMap>

DFM_MOUNT_BEGIN_NS

class DFMDeviceManagerPrivate final
{
public:
    DFMDeviceManagerPrivate(DFMDeviceManager *qq);

    template<typename DFMSubMonitor, typename... ConstructArgs>
    bool registerMonitor(ConstructArgs &&... args);
    QSharedPointer<DFMMonitor> getRegisteredMonitor(DeviceType type) const;
    bool startMonitor();
    bool stopMonitor();
    QMap<DeviceType, QStringList> devices(DeviceType type);

    QMap<DeviceType, QSharedPointer<DFMMonitor>> monitors;
    MonitorError lastError;

    DFMDeviceManager *q = nullptr;
};

DFM_MOUNT_END_NS

#endif   // DFMDEVICEMANAGERPRIVATE_H
