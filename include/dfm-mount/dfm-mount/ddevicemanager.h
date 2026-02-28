// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDEVICEMANAGER_H
#define DDEVICEMANAGER_H

#include <dfm-mount/base/dmount_global.h>

#include <QObject>

DFM_MOUNT_BEGIN_NS

class DDeviceMonitor;
class DDevice;
class DDeviceManagerPrivate;
class DDeviceManager final : public QObject
{
    Q_OBJECT

public:
    static DDeviceManager *instance();

    template<class DFMSubMonitor, typename... ConstructArgs>
    bool registerMonitor(ConstructArgs &&...);
    QSharedPointer<DDeviceMonitor> getRegisteredMonitor(DeviceType type) const;
    bool startMonitorWatch();
    bool stopMonitorWatch();
    MonitorError lastError() const;
    QMap<DeviceType, QStringList> devices(DeviceType type = DeviceType::kAllDevice);

Q_SIGNALS:
    // these signals are transferred from monitors, you can use them or connect directly from monitors,
    // but if you want to cannect some special signals, find them in specific monitor.
    void deviceAdded(const QString &deviceKey, DeviceType type);
    void deviceRemoved(const QString &deviceKey, DeviceType type);
    void mounted(const QString &deviceKey, const QString &mountPoint, DeviceType type);
    void unmounted(const QString &deviceKey, DeviceType type);
    void propertyChanged(const QString &deviceKey, const QMap<Property, QVariant> &changes, DeviceType type);

private:
    DDeviceManager(QObject *parent = nullptr);
    ~DDeviceManager();

    QScopedPointer<DDeviceManagerPrivate> d;
};

DFM_MOUNT_END_NS

#endif   // DDEVICEMANAGER_H
