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

#include "base/ddevice.h"
#include "ddevicemanager.h"
#include "dblockmonitor.h"
#include "private/ddevicemanager_p.h"

#include <QDebug>

DFM_MOUNT_USE_NS

DDeviceManagerPrivate::DDeviceManagerPrivate(DDeviceManager *qq)
    : q(qq)
{
}

QSharedPointer<DDeviceMonitor> DDeviceManagerPrivate::getRegisteredMonitor(DeviceType type) const
{
    if (type == DeviceType::kAllDevice) {
        qWarning() << "DeviceType::AllDevice is not a specific type.";
        return nullptr;
    }
    return monitors.value(type, nullptr);
}

bool DDeviceManagerPrivate::startMonitor()
{
    bool res = true;
    for (const auto &monitor : monitors) {
        auto type = monitor->monitorObjectType();
        res &= monitor->startMonitor();
        if (res)
            qDebug() << type << "started...";
        else
            qWarning() << type << "failed to start...";
    }
    return res;
}

bool DDeviceManagerPrivate::stopMonitor()
{
    bool res = true;
    for (const auto &monitor : monitors) {
        auto type = monitor->monitorObjectType();
        res &= monitor->stopMonitor();
        if (res)
            qDebug() << type << "stopped...";
        else
            qWarning() << type << "failed to stop...";
    }
    return res;
}

QMap<DeviceType, QStringList> DDeviceManagerPrivate::devices(DeviceType type)
{
    auto getAllDev = [this] {
        QMap<DeviceType, QStringList> ret;
        for (const auto &monitor : monitors) {
            if (monitor)
                ret.insert(monitor->monitorObjectType(), monitor->getDevices());
            else
                lastError = MonitorError::kMonitorNotRegister;
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

        lastError = MonitorError::kMonitorNotRegister;
        return ret;
    };

    switch (type) {
    case DeviceType::kAllDevice:
        return getAllDev();
    case DeviceType::kNetDevice:   // by intentionally
    case DeviceType::kProtocolDevice:
    case DeviceType::kBlockDevice:
        return getDevsOfType(type);
    }

    return {};
}

DDeviceManager::DDeviceManager(QObject *parent)
    : QObject(parent), d(new DDeviceManagerPrivate(this))
{
    registerMonitor<DBlockMonitor>(this);
    registerMonitor<DProtocolMonitor>(this);
}

DDeviceManager::~DDeviceManager()
{
}

DDeviceManager *DDeviceManager::instance()
{
    static DDeviceManager mng;
    return &mng;
}

QSharedPointer<DDeviceMonitor> DDeviceManager::getRegisteredMonitor(DeviceType type) const
{
    return d->getRegisteredMonitor(type);
}

bool DDeviceManager::startMonitorWatch()
{
    return d->startMonitor();
}

bool DDeviceManager::stopMonitorWatch()
{
    return d->stopMonitor();
}

MonitorError DDeviceManager::lastError() const
{
    return d->lastError;
}

QMap<DeviceType, QStringList> DDeviceManager::devices(DeviceType type)
{
    return d->devices(type);
}

template<class DFMSubMonitor, typename... ConstructArgs>
bool DDeviceManager::registerMonitor(ConstructArgs &&... args)
{
    return d->registerMonitor<DFMSubMonitor>(std::forward<ConstructArgs>(args)...);
}

template<typename DFMSubMonitor, typename... ConstructArgs>
bool DDeviceManagerPrivate::registerMonitor(ConstructArgs &&... args)
{
    QSharedPointer<DDeviceMonitor> monitor(new DFMSubMonitor(std::forward<ConstructArgs>(args)...));
    if (!monitor) return false;

    auto type = monitor->monitorObjectType();
    if (monitors.contains(type)) {
        lastError = MonitorError::kMonitorAlreadyRegistered;
        return false;
    }
    monitors.insert(type, monitor);

    QObject::connect(monitor.data(), &DDeviceMonitor::deviceAdded, q, [type, this](const QString &devId) {
        Q_EMIT q->deviceAdded(devId, type);
    });
    QObject::connect(monitor.data(), &DDeviceMonitor::deviceRemoved, q, [type, this](const QString &devId) {
        Q_EMIT q->deviceRemoved(devId, type);
    });
    QObject::connect(monitor.data(), &DDeviceMonitor::mountAdded, q, [type, this](const QString &devId, const QString &mpt) {
        Q_EMIT q->mounted(devId, mpt, type);
    });
    QObject::connect(monitor.data(), &DDeviceMonitor::mountRemoved, q, [type, this](const QString &devId) {
        Q_EMIT q->unmounted(devId, type);
    });
    QObject::connect(monitor.data(), &DDeviceMonitor::propertyChanged, q, [type, this](const QString &devId, const QMap<Property, QVariant> &changes) {
        Q_EMIT q->propertyChanged(devId, changes, type);
    });

    return true;
}
