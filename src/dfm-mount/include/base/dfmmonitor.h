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

public:
    DFM_MNT_VIRTUAL bool startMonitor();
    DFM_MNT_VIRTUAL bool stopMonitor();
    DFM_MNT_VIRTUAL MonitorStatus status() const;
    DFM_MNT_VIRTUAL DeviceType monitorObjectType() const;
    DFM_MNT_VIRTUAL QStringList getDevices() const;
    DFM_MNT_VIRTUAL QSharedPointer<DFMDevice> createDeviceById(const QString &id) const;

public:
    using StartMonitorFunc      = std::function<bool ()>;
    using StopMonitorFunc       = std::function<bool ()>;
    using StatusFunc            = std::function<MonitorStatus ()>;
    using MonitorObjectTypeFunc = std::function<DeviceType ()>;
    using GetDevicesFunc        = std::function<QStringList ()>;
    using CreateDeviceByIdFunc  = std::function<QSharedPointer<DFMDevice> (const QString &)>;

    void registerStartMonitor(const StartMonitorFunc &func);
    void registerStopMonitor(const StopMonitorFunc &func);
    void registerStatus(const StatusFunc &func);
    void registerMonitorObjectType(const MonitorObjectTypeFunc &func);
    void registerGetDevices(const GetDevicesFunc &func);
    void registerCreateDeviceById(const CreateDeviceByIdFunc &func);

protected:
    DFMMonitor(DFMMonitorPrivate *dd, QObject *parent);
    virtual ~DFMMonitor();

Q_SIGNALS:
    // TODO: complete protocol device id.
    /*!
     * \brief deviceAdded
     * \param deviceId  for block devices, this id is block object path in udisks, but for protocol devices, it's not.
     */
    void deviceAdded(const QString &deviceId);
    void deviceRemoved(const QString &deviceId);
    void mountAdded(const QString &deviceId, const QString &mountPoint);
    void mountRemoved(const QString &deviceId);
    void propertyChanged(const QString &deviceId, const QMap<Property, QVariant> &changes);

protected:
    QScopedPointer<DFMMonitorPrivate> d;
};

DFM_MOUNT_END_NS

#endif // DFMMONITOR_H
