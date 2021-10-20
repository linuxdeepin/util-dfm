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

    bool registerMonitor(DeviceType type, DFMMonitor *monitor);
    // this object is owned by manager, DO NOT free it.
    DFMMonitor *getRegisteredMonitor(DeviceType type) const;
    bool startMonitorWatch();
    bool stopMonitorWatch();
    MonitorError lastError() const;

    // the objects are owned by manager, do not free it manually
    QList<DFMDevice *> devices(DeviceType type = DeviceType::AllDevice);

Q_SIGNALS:
    void deviceAdded(DFMDevice *dev);
    /*!
     * \brief deviceRemoved
     * \param path for block device the path is dbus object path.
     */
    void deviceRemoved(const QString &path);
    void mounted(DFMDevice *dev, const QString &mountPoint);
    void unmounted(DFMDevice *dev);
    void propertyChanged(DFMDevice *dev, const QMap<Property, QVariant> &changes);


private:
    DFMDeviceManager(QObject *parent = nullptr);
    ~DFMDeviceManager();

    QScopedPointer<DFMDeviceManagerPrivate> d;

};

DFM_MOUNT_END_NS

#endif // DFMDEVICEMANAGER_H
