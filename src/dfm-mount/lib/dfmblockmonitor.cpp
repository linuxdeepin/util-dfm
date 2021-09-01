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

#include "dfmblockmonitor.h"
#include "private/dfmblockmonitor_p.h"
#include "dfmblockdevice.h"

#include <QDebug>

DFM_MOUNT_USE_NS

DFMBlockMonitorPrivate::DFMBlockMonitorPrivate(DFMBlockMonitor *qq)
    : q_ptr(qq)
{
    qq->registerStartMonitor(std::bind(&DFMBlockMonitorPrivate::startMonitor, this));
    qq->registerStopMonitor(std::bind(&DFMBlockMonitorPrivate::stopMonitor, this));
    qq->registerStatus(std::bind(&DFMBlockMonitorPrivate::status, this));
    qq->registerMonitorObjectType(std::bind(&DFMBlockMonitorPrivate::monitorObjectType, this));
}

bool DFMBlockMonitorPrivate::startMonitor()
{
    if (client)
        return true;

    GError *err = nullptr;
    client = udisks_client_new_sync(nullptr, &err);
    if (!client) {
        if (err) {
            const QString && errMsg = err->message;
            qCritical() << "start monitor block error: " << errMsg;
            g_error_free(err);
        }
        return false;
    }

    GDBusObjectManager *dbusMng = udisks_client_get_object_manager(client);
    if (!dbusMng) {
        g_object_unref(client);
        client = nullptr;
        qCritical() << "start monitor block failed: cannot get dbus monitor";
        return false;
    }

    g_signal_connect(dbusMng, "object-added", G_CALLBACK(&DFMBlockMonitorPrivate::onObjectAdded), q_ptr);
    g_signal_connect(dbusMng, "object_removed", G_CALLBACK(&DFMBlockMonitorPrivate::onObjectRemoved), q_ptr);
    g_signal_connect(dbusMng, "interface-proxy-properties-changed", G_CALLBACK(&DFMBlockMonitorPrivate::onPropertyChanged), q_ptr);

    curStatus = MonitorStatus::Monitoring;
    return true;
}

bool DFMBlockMonitorPrivate::stopMonitor()
{
    if (client) {
        g_object_unref(client);
        client = nullptr;
    }
    curStatus = MonitorStatus::Idle;
    return true;
}

MonitorStatus DFMBlockMonitorPrivate::status() const
{
    return curStatus;
}

DeviceType DFMBlockMonitorPrivate::monitorObjectType() const
{
    return DeviceType::BlockDevice;
}

void DFMBlockMonitorPrivate::onObjectAdded(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    DFMBlockMonitor *monitor = static_cast<DFMBlockMonitor *>(userData);
    if (!monitor)
        return;

    qDebug() << __PRETTY_FUNCTION__;
    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;
    UDisksBlock *block = udisks_object_get_block(udisksObj);
    if (block) {
        qDebug() << "blockDeviceAdded...";
        qDebug() << "\t\t\t" << udisks_block_get_device(block);
        QString dev = udisks_block_get_device(block);
        Q_EMIT monitor->deviceAdded(new DFMBlockDevice(dev, monitor));
        return;
    }
    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    if (drive) {
        qDebug() << "driveAdded";
        return;
    }
    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);
    if (partition) {
        qDebug() << "partitionAdded";
        return;
    }
}

void DFMBlockMonitorPrivate::onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    qDebug() << __PRETTY_FUNCTION__;
}

void DFMBlockMonitorPrivate::onPropertyChanged(GDBusObjectManagerClient *mngClient, GDBusObjectProxy *objProxy, GDBusProxy *dbusProxy, GVariant *property, const gchar * const invalidProperty, gpointer *userData)
{
    qDebug() << __PRETTY_FUNCTION__;
}

DFMBlockMonitor::DFMBlockMonitor(QObject *parent)
    : DFMMonitor (parent),
      d_pointer(new DFMBlockMonitorPrivate(this))
{

}

DFMBlockMonitor::~DFMBlockMonitor()
{

}
