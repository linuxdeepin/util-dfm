// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBLOCKMONITOR_P_H
#define DBLOCKMONITOR_P_H

#include <dfm-mount/dblockmonitor.h>

#include "private/ddevicemonitor_p.h"

#include <QMap>
#include <QSet>
#include <QDBusServiceWatcher>

extern "C" {
#include <udisks/udisks.h>
}

DFM_MOUNT_BEGIN_NS

#define OBJECT_ADDED "object-added"
#define OBJECT_REMOVED "object-removed"
#define INTERFACE_ADDED "interface-added"
#define INTERFACE_REMOVED "interface-removed"
#define PROPERTY_CHANGED "interface-proxy-properties-changed"

class DBlockDevice;
class DBlockMonitorPrivate final : public DDeviceMonitorPrivate
{
public:
    DBlockMonitorPrivate(DBlockMonitor *qq);
    ~DBlockMonitorPrivate();

    bool startMonitor() DMNT_OVERRIDE;
    bool stopMonitor() DMNT_OVERRIDE;
    bool startDeviceMonitor();
    bool stopDeviceMonitor();
    DeviceType monitorObjectType() const DMNT_OVERRIDE;
    QStringList getDevices() DMNT_OVERRIDE;
    QSharedPointer<DDevice> createDeviceById(const QString &id) DMNT_OVERRIDE;

    QStringList resolveDevice(const QVariantMap &devspec, const QVariantMap &opts);
    QStringList resolveDeviceNode(const QString &node, const QVariantMap &opts);
    QStringList resolveDeviceOfDrive(const QString &drvObjPath);

private:
    static void onObjectAdded(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData);
    static void onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData);
    static void onPropertyChanged(GDBusObjectManagerClient *mngClient, GDBusObjectProxy *objProxy, GDBusProxy *dbusProxy,
                                  GVariant *property, const gchar *const invalidProperty, gpointer userData);
    static void onInterfaceAdded(GDBusObjectManager *mng, GDBusObject *obj, GDBusInterface *iface, gpointer userData);
    static void onInterfaceRemoved(GDBusObjectManager *mng, GDBusObject *obj, GDBusInterface *iface, gpointer userData);

    void initDevices();

public:
    UDisksClient *client = nullptr;
    QDBusServiceWatcher *watcher = nullptr;

    static QMap<QString, QSet<QString>> blksOfDrive;
};

DFM_MOUNT_END_NS

#endif   // DBLOCKMONITOR_P_H
