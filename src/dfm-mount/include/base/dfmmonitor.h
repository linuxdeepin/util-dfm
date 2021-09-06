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
#ifndef DFMMONITOR_H
#define DFMMONITOR_H

#include "dfmmount_global.h"

#include <QObject>
#include <QSharedPointer>

#include <functional>

DFM_MOUNT_BEGIN_NS

class DFMDevice;
class DFMMonitorPrivate;
class DFMMonitor: public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DFMMonitor)

public:
    DFMMonitor(QObject *parent);
    virtual ~DFMMonitor(){}

    DFM_MNT_VIRTUAL bool startMonitor();
    DFM_MNT_VIRTUAL bool stopMonitor();
    DFM_MNT_VIRTUAL MonitorStatus status() const;
    DFM_MNT_VIRTUAL DeviceType monitorObjectType() const;
    DFM_MNT_VIRTUAL QList<DFMDevice *> getDevices() const;

public:
    using StartMonitorFunc      = std::function<bool ()>;
    using StopMonitorFunc       = std::function<bool ()>;
    using StatusFunc            = std::function<MonitorStatus ()>;
    using MonitorObjectTypeFunc = std::function<DeviceType ()>;
    using GetDevicesFunc        = std::function<QList<DFMDevice *> ()>;

    void registerStartMonitor(const StartMonitorFunc &func);
    void registerStopMonitor(const StopMonitorFunc &func);
    void registerStatus(const StatusFunc &func);
    void registerMonitorObjectType(const MonitorObjectTypeFunc &func);
    void registerGetDevices(const GetDevicesFunc &func);

Q_SIGNALS:
    void driveAdded();
    void driveRemoved();
    void deviceAdded(DFMDevice *dev);
    void deviceRemoved(DFMDevice *dev);
    void mountAdded(const QString &mountPoint);
    void mountRemoved(const QString &mountPoint);
    void propertiesChanged(const QVariantMap &);

protected:
    DFMMonitor(DFMMonitorPrivate &dd, QObject *parent = nullptr);
};

DFM_MOUNT_END_NS

#endif // DFMMONITOR_H
