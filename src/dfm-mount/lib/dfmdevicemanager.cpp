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

#include "base/dfmdevice.h"
#include "dfmdevicemanager.h"
#include "private/dfmdevicemanager_p.h"
#include "dfmblockmonitor.h"

#include <QDebug>

DFM_MOUNT_USE_NS

DFMDeviceManagerPrivate::DFMDeviceManagerPrivate(DFMDeviceManager *qq)
    : q(qq)
{
}

QSharedPointer<DFMMonitor> DFMDeviceManagerPrivate::getRegisteredMonitor(DeviceType type) const
{
    if (type == DeviceType::AllDevice) {
        qWarning() << "DeviceType::AllDevice is not a specific type.";
        return nullptr;
    }
    return monitors.value(type, nullptr);
}

bool DFMDeviceManagerPrivate::startMonitor()
{
    bool res = true;
    for (const auto &monitor : monitors) {
        int type = static_cast<int>(monitor->monitorObjectType());
        res &= monitor->startMonitor();
        if (res)
            qDebug() << type << "started...";
        else
            qWarning() << type << "failed to start...";
    }
    return res;
}

bool DFMDeviceManagerPrivate::stopMonitor()
{
    bool res = true;
    for (const auto &monitor : monitors) {
        int type = static_cast<int>(monitor->monitorObjectType());
        res &= monitor->stopMonitor();
        if (res)
            qDebug() << type << "stopped...";
        else
            qWarning() << type << "failed to stop...";
    }
    return res;
}

QMap<DeviceType, QStringList> DFMDeviceManagerPrivate::devices(DeviceType type)
{
    auto getAllDev = [this] {
        QMap<DeviceType, QStringList> ret;
        for (const auto &monitor : monitors) {
            if (monitor)
                ret.insert(monitor->monitorObjectType(), monitor->getDevices());
            else
                lastError = MonitorError::MonitorNotRegister;
        }
        return ret;
    };

    auto getDevsOfType = [this](DeviceType type) {
        QMap<DeviceType, QStringList> ret;
        auto monitor = monitors.value(type);
        if (monitor) {
            ret.insert(type, monitor->getDevices());
            return ret;
        }

        lastError = MonitorError::MonitorNotRegister;
        return ret;
    };

    switch (type) {
    case DeviceType::AllDevice:
        return getAllDev();
    case DeviceType::NetDevice:   // by intentionally
    case DeviceType::ProtocolDevice:
    case DeviceType::BlockDevice:
        return getDevsOfType(type);
    }

    return {};
}

DFMDeviceManager::DFMDeviceManager(QObject *parent)
    : QObject(parent), d(new DFMDeviceManagerPrivate(this))
{
    registerMonitor<DFMBlockMonitor>(this);
    registerMonitor<DFMProtocolMonitor>(this);
}

DFMDeviceManager::~DFMDeviceManager()
{
}

DFMDeviceManager *DFMDeviceManager::instance()
{
    static DFMDeviceManager mng;
    return &mng;
}

QSharedPointer<DFMMonitor> DFMDeviceManager::getRegisteredMonitor(DeviceType type) const
{
    return d->getRegisteredMonitor(type);
}

bool DFMDeviceManager::startMonitorWatch()
{
    return d->startMonitor();
}

bool DFMDeviceManager::stopMonitorWatch()
{
    return d->stopMonitor();
}

MonitorError DFMDeviceManager::lastError() const
{
    return d->lastError;
}

QMap<DeviceType, QStringList> DFMDeviceManager::devices(DeviceType type)
{
    return d->devices(type);
}

template<class DFMSubMonitor, typename... ConstructArgs>
bool DFMDeviceManager::registerMonitor(ConstructArgs &&... args)
{
    return d->registerMonitor<DFMSubMonitor>(std::forward<ConstructArgs>(args)...);
}

template<typename DFMSubMonitor, typename... ConstructArgs>
bool DFMDeviceManagerPrivate::registerMonitor(ConstructArgs &&... args)
{
    QSharedPointer<DFMMonitor> monitor(new DFMSubMonitor(std::forward<ConstructArgs>(args)...));
    if (!monitor) return false;

    auto type = monitor->monitorObjectType();
    if (monitors.contains(type)) {
        lastError = MonitorError::MonitorAlreadyRegistered;
        return false;
    }
    monitors.insert(type, monitor);

    QObject::connect(monitor.data(), &DFMMonitor::deviceAdded, q, [type, this](const QString &devId) {
        Q_EMIT q->deviceAdded(devId, type);
    });
    QObject::connect(monitor.data(), &DFMMonitor::deviceRemoved, q, [type, this](const QString &devId) {
        Q_EMIT q->deviceRemoved(devId, type);
    });
    QObject::connect(monitor.data(), &DFMMonitor::mountAdded, q, [type, this](const QString &devId, const QString &mpt) {
        Q_EMIT q->mounted(devId, mpt, type);
    });
    QObject::connect(monitor.data(), &DFMMonitor::mountRemoved, q, [type, this](const QString &devId) {
        Q_EMIT q->unmounted(devId, type);
    });
    QObject::connect(monitor.data(), &DFMMonitor::propertyChanged, q, [type, this](const QString &devId, const QMap<Property, QVariant> &changes) {
        Q_EMIT q->propertyChanged(devId, changes, type);
    });

    return true;
}
