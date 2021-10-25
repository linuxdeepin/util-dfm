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
#ifndef DFMBLOCKMONITOR_P_H
#define DFMBLOCKMONITOR_P_H

#include "dfmblockmonitor.h"
#include "private/dfmmonitor_p.h"

#include <QMap>

#include <udisks/udisks.h>

DFM_MOUNT_BEGIN_NS

#define OBJECT_ADDED        "object-added"
#define OBJECT_REMOVED      "object-removed"
#define PROPERTY_CHANGED    "interface-proxy-properties-changed"

class DFMBlockDevice;
class DFMBlockMonitorPrivate final: public DFMMonitorPrivate
{
public:
    DFMBlockMonitorPrivate(DFMBlockMonitor *qq);
    ~DFMBlockMonitorPrivate();

    bool startMonitor() DFM_MNT_OVERRIDE;
    bool stopMonitor() DFM_MNT_OVERRIDE;
    MonitorStatus status() const DFM_MNT_OVERRIDE;
    DeviceType monitorObjectType() const DFM_MNT_OVERRIDE;
    QStringList getDevices() DFM_MNT_OVERRIDE;
    QSharedPointer<DFMDevice> createDeviceById(const QString &id) DFM_MNT_OVERRIDE;

    QStringList resolveDevice(const QVariantMap &devspec, const QVariantMap &opts);
    QStringList resolveDeviceNode(const QString &node, const QVariantMap &opts);
    QStringList resolveDeviceOfDrive(const QString &drvObjPath);

private:
    static void onObjectAdded(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData);
    static void onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData);
    static void onPropertyChanged(GDBusObjectManagerClient *mngClient, GDBusObjectProxy *objProxy, GDBusProxy *dbusProxy,
                                  GVariant *property, const gchar *const invalidProperty, gpointer userData);

public:
    UDisksClient *client = nullptr;
    MonitorStatus curStatus = MonitorStatus::Idle;

//    QMap<QString, QStringList> devicesOfDrive;
    QMap<QString, ulong> connections;
    static const QMap<QString, Property> propertyName2Property;
};

DFM_MOUNT_END_NS

#endif // DFMBLOCKMONITOR_P_H
