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
#ifndef DDEVICEMONITOR_H
#define DDEVICEMONITOR_H

#include "dmount_global.h"

#include <QObject>
#include <QSharedPointer>

#include <functional>

DFM_MOUNT_BEGIN_NS

class DDevice;
class DDeviceMonitorPrivate;
class DDeviceMonitor : public QObject
{
    Q_OBJECT

public:
    MonitorStatus status() const;

    DMNT_VIRTUAL bool startMonitor();
    DMNT_VIRTUAL bool stopMonitor();
    DMNT_VIRTUAL DeviceType monitorObjectType() const;
    DMNT_VIRTUAL QStringList getDevices() const;
    DMNT_VIRTUAL QSharedPointer<DDevice> createDeviceById(const QString &id) const;

public:
    using StartMonitorFunc = std::function<bool()>;
    using StopMonitorFunc = std::function<bool()>;
    using MonitorObjectTypeFunc = std::function<DeviceType()>;
    using GetDevicesFunc = std::function<QStringList()>;
    using CreateDeviceByIdFunc = std::function<QSharedPointer<DDevice>(const QString &)>;

    void registerStartMonitor(const StartMonitorFunc &func);
    void registerStopMonitor(const StopMonitorFunc &func);
    void registerMonitorObjectType(const MonitorObjectTypeFunc &func);
    void registerGetDevices(const GetDevicesFunc &func);
    void registerCreateDeviceById(const CreateDeviceByIdFunc &func);

protected:
    DDeviceMonitor(DDeviceMonitorPrivate *dd, QObject *parent);
    virtual ~DDeviceMonitor();

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
    QScopedPointer<DDeviceMonitorPrivate> d;
};

DFM_MOUNT_END_NS

#endif   // DDEVICEMONITOR_H
