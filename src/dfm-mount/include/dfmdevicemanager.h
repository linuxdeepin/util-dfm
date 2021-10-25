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
#ifndef DFMDEVICEMANAGER_H
#define DFMDEVICEMANAGER_H

#include "base/dfmmount_global.h"
#include "base/dfmmountdefines.h"

#include <QObject>

DFM_MOUNT_BEGIN_NS

class DFMMonitor;
class DFMDevice;
class DFMDeviceManagerPrivate;
class DFMDeviceManager final : public QObject
{
    Q_OBJECT

public:
    static DFMDeviceManager *instance();

    template<class DFMSubMonitor, typename... ConstructArgs>
    bool registerMonitor(ConstructArgs&&...);
    QSharedPointer<DFMMonitor> getRegisteredMonitor(DeviceType type) const;
    bool startMonitorWatch();
    bool stopMonitorWatch();
    MonitorError lastError() const;
    QMap<DeviceType, QStringList> devices(DeviceType type = DeviceType::AllDevice);

Q_SIGNALS:
    // these signals are transferred from monitors, you can use them or connect directly from monitors,
    // but if you want to cannect some special signals, find them in specific monitor.
    void deviceAdded(const QString &deviceKey, DeviceType type);
    void deviceRemoved(const QString &deviceKey, DeviceType type);
    void mounted(const QString &deviceKey, const QString &mountPoint, DeviceType type);
    void unmounted(const QString &deviceKey, DeviceType type);
    void propertyChanged(const QString &deviceKey, const QMap<Property, QVariant> &changes, DeviceType type);


private:
    DFMDeviceManager(QObject *parent = nullptr);
    ~DFMDeviceManager();

    QScopedPointer<DFMDeviceManagerPrivate> d;

};

DFM_MOUNT_END_NS

#endif // DFMDEVICEMANAGER_H
