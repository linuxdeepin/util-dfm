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
#ifndef DBLOCKMONITOR_P_H
#define DBLOCKMONITOR_P_H

#include "dblockmonitor.h"
#include "private/ddevicemonitor_p.h"

#include <QMap>
#include <QSet>

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

    static QMap<QString, QSet<QString>> blksOfDrive;
};

DFM_MOUNT_END_NS

#endif   // DBLOCKMONITOR_P_H
