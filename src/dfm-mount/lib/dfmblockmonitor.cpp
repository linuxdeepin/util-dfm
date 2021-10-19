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
#include "private/dfmblockdevice_p.h"
#include "base/dfmmountdefines.h"

#include <QDebug>
#include <QMapIterator>

DFM_MOUNT_USE_NS

DFMBlockMonitorPrivate::DFMBlockMonitorPrivate(DFMBlockMonitor *qq)
    : DFMMonitorPrivate (qq)
{
    GError *err = nullptr;
    client = udisks_client_new_sync(nullptr, &err);
    if (err) {
        qCritical() << "init udisks client failed. " << err->message;
        g_error_free(err);
    }
}

DFMBlockMonitorPrivate::~DFMBlockMonitorPrivate()
{
    qDebug() << "block monitor release...";
    if (client) {
        g_object_unref(client);
        client = nullptr;
    }

    // clear the caches and release the mems.
    for (auto key: devices.keys())
        delete devices[key];
    devices.clear();

    // the values of drivers which we use *_PEEK_* to obtain them belong to UDisksClient, they are just refs, do not free/unref it.
    // which is declared on the API page of UDisksObject:
    // http://storaged.org/doc/udisks2-api/latest/UDisksObject.html#udisks-object-peek-drive
    drives.clear();
    // the values of devicesOfDrive are just a ref of DFMBlockDevice, which has been released above, so do not free it too.
    devicesOfDrive.clear();
}

bool DFMBlockMonitorPrivate::startMonitor()
{
    if (!client) {
        qCritical() << "client is not valid";
        return false;
    }

    // the mng is owned by client, do not free it manually
    GDBusObjectManager *dbusMng = udisks_client_get_object_manager(client);
    if (!dbusMng) {
        qCritical() << "start monitor block failed: cannot get dbus monitor";
        return false;
    }

    connections.insert(OBJECT_ADDED,
                       g_signal_connect(dbusMng, OBJECT_ADDED, G_CALLBACK(&DFMBlockMonitorPrivate::onObjectAdded), this));
    connections.insert(OBJECT_REMOVED,
                       g_signal_connect(dbusMng, OBJECT_REMOVED, G_CALLBACK(&DFMBlockMonitorPrivate::onObjectRemoved), this));
    connections.insert(PROPERTY_CHANGED,
                       g_signal_connect(dbusMng, PROPERTY_CHANGED, G_CALLBACK(&DFMBlockMonitorPrivate::onPropertyChanged), this));

    curStatus = MonitorStatus::Monitoring;
    qDebug() << "monitor started...";
    return true;
}

bool DFMBlockMonitorPrivate::stopMonitor()
{
    if (!client) {
        qDebug() << "client is not valid";
        return false;
    }

    GDBusObjectManager *dbusMng = udisks_client_get_object_manager(client);
    for (auto iter = connections.cbegin(); iter != connections.cend(); iter++)
        g_signal_handler_disconnect(dbusMng, iter.value());
    connections.clear();

    curStatus = MonitorStatus::Idle;
    qDebug() << "monitor stopped...";
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

QList<DFMDevice *> DFMBlockMonitorPrivate::getDevices()
{
    getAllDevs();
    QList<DFMDevice *> ret;
    for (auto iter = devices.cbegin(); iter != devices.cend(); iter++)
        ret.push_back(iter.value());
    return ret;
}

void DFMBlockMonitorPrivate::getAllDevs()
{
    Q_ASSERT_X(client, __FUNCTION__, "client must be valid to get all devices");
    UDisksManager *mng = udisks_client_get_manager(client);
    if (!mng) {
        qWarning() << "udisks client manager is not valid, cannot get devices";
        return;
    }

    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manually.
    GVariant *gOpts = g_variant_builder_end(builder);

    char **blkObjs = nullptr;
    GError *err = nullptr;
    bool ok = udisks_manager_call_get_block_devices_sync(mng, gOpts, &blkObjs, nullptr, &err);
    if (ok && blkObjs) {
        int next = 0;
        while (blkObjs && blkObjs[next]) {
            char *blkObjPath = blkObjs[next];
            UDisksObject *blockObject = udisks_client_peek_object(client, blkObjPath);
            if (!blockObject)
                continue;

            UDisksBlock *block = udisks_object_peek_block(blockObject);
            UDisksFilesystem *filesystem = udisks_object_peek_filesystem(blockObject);
            UDisksPartition *partition = udisks_object_peek_partition(blockObject);

            if (block) {
                DFMBlockDevice *dev = nullptr;
                // if already contained this block, update handlers.
                if (this->devices.contains(blkObjPath))
                    dev = this->devices[blkObjPath];
                else
                    dev = new DFMBlockDevice(q);

                auto blkD = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(dev->d.data());
                blkD->blockHandler = block;

                if (filesystem)
                    blkD->fileSystemHandler = filesystem;

                if (partition)
                    blkD->partitionHandler = partition;

                char *driveObjPath = udisks_block_dup_drive(block);
                UDisksObject *driveObject = udisks_client_peek_object(client, driveObjPath);

                if (driveObject) {
                    UDisksDrive *drive = udisks_object_peek_drive(driveObject);
                    if (drive) {
                        blkD->driveHandler = drive;
                        this->drives.insert(QString(driveObjPath), drive);
                    }
                }

                this->devices.insert(blkObjPath, dev);
                g_free(driveObjPath);

            }
            next += 1;
        }
        g_strfreev(blkObjs);
    }

    if (err) {
        // TODO: we need more error handle here
        g_error_free(err);
    }
}

void DFMBlockMonitorPrivate::onObjectAdded(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);
    DFMBlockMonitorPrivate *d = static_cast<DFMBlockMonitorPrivate *>(userData);
    Q_ASSERT_X(d, __FUNCTION__, "monitor is not valid");

    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;

    UDisksJob *job = udisks_object_peek_job(udisksObj);
    if (job)
        return;

    QString objKey = g_dbus_object_get_object_path(obj);

    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    if (drive) {
        qDebug() << "driveAdded";
        d->drives.insert(objKey, drive);
        // Q_EMIT monitor->driveAdded
    }

    UDisksBlock *block = udisks_object_peek_block(udisksObj);
    UDisksFilesystem *fileSystem = udisks_object_peek_filesystem(udisksObj);
    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);

    DFMBlockDevice *blkDev = nullptr;
    if (block) {
        blkDev = new DFMBlockDevice(d->q);
        auto blkD = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(blkDev->d.data());
        blkD->blockHandler = block;
        QString drive = blkDev->getProperty(Property::BlockDrive).toString();
        if (d->drives.contains(drive)) {
            blkD->driveHandler = d->drives.value(drive);
        }

        if (fileSystem) {
           blkD->fileSystemHandler = fileSystem;
            // Q_EMIT monitor->filesystemadded
        }
        if (partition) {
            blkD->partitionHandler = partition;
            // Q_EMIT monitor->partitionadded
        }

        Q_EMIT d->q->deviceAdded(blkDev);
        d->devices.insert(objKey, blkDev);
    }
}

void DFMBlockMonitorPrivate::onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);

    DFMBlockMonitorPrivate *d = static_cast<DFMBlockMonitorPrivate *>(userData);
    Q_ASSERT_X(d, __FUNCTION__, "monitor is not valid");

    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;

    // in past time, we never care about the UDisksJob, so we ignore it here.
    UDisksJob *job = udisks_object_peek_job(udisksObj);
    if (job)
        return;

    QString objKey = g_dbus_object_get_object_path(obj);
//    qDebug() << "\t" << objKey;

    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    if (drive) {
        qDebug() << "driveRemoved";
        d->drives.remove(objKey);
        // Q_EMIT monitor->driveRemoved
    }

    UDisksBlock *block = udisks_object_get_block(udisksObj);
//    UDisksFilesystem *fileSystem = udisks_object_peek_filesystem(udisksObj);
//    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);
    if (block) {
        // we get the handler by *_GET_* funcs, so there is no need to release it by ourselves.
        auto *dev = d->devices.take(objKey);
        delete dev;
        // Q_EMIT monitor->deviceRemoved;
    }
}

void DFMBlockMonitorPrivate::onPropertyChanged(GDBusObjectManagerClient *mngClient, GDBusObjectProxy *dbusObjProxy, GDBusProxy *dbusProxy,
                                               GVariant *property, const gchar * const invalidProperty, gpointer userData)
{
    Q_UNUSED(mngClient);
    Q_UNUSED(dbusObjProxy);
    Q_UNUSED(invalidProperty);

    DFMBlockMonitorPrivate *d = static_cast<DFMBlockMonitorPrivate *>(userData);
    Q_ASSERT_X(d, __FUNCTION__, "monitor is not valid");

    // obtain the object path like "/org/freedesktop/UDisks2/block_devices/sdb1" from dbusProxy
    // and which is the key of DFMBlockMonitorPrivate::devices
    QString objPath = dbusProxy ? QString(g_dbus_proxy_get_object_path(dbusProxy)) : QString();
    if (objPath.isEmpty())
        return;

    // obtain the DFMDevice(s) handler to emit the propertyChanged signal
    QList<DFMBlockDevice *> devs;
    if (objPath.startsWith("/org/freedesktop/UDisks2/drives/")) { // means this is a DRIVE objech which property has changed
        qDebug() << "drive changed...";
        // TODO: we need to find a way to notify apps that the drivers' properties have changed
        // but for now, we can just ignore it, there is not so much properties of drives' that we shall concern about.
        devs = d->devicesOfDrive.values(objPath);
    } else { // we treat it as Blocks
        DFMBlockDevice *dev = d->devices.value(objPath, nullptr);
        if (dev)
            devs << dev;
    }
    if (devs.isEmpty()) // since there is no device for this object, so there is no need to do subsequent works
        return;

    GVariantIter *iter = nullptr;
    const char *property_name = nullptr;
    GVariant *value = nullptr;

    // iterate the changed key-values of this object, it is storaged like dictionaries. the a{sv} means array of String-Variant pair.
    g_variant_get (property, "a{sv}", &iter);
    while (g_variant_iter_next(iter, "{&sv}", &property_name, &value)) {
        char *value_str;
        value_str = g_variant_print(value, FALSE);

        qDebug() << property_name << "\t\tchanged, the new value is: " << value_str;
        // TODO: we need find a way to map the property_name and the Property enum, so that we can find the right enum quickly.

        // handle the signal emits here.
        QList<DFMBlockDevice *>::const_iterator iter;
        for (iter = devs.cbegin(); iter != devs.cend(); iter += 1) {
            // TODO: emits the signal
        }
    }

    // TODO: when mountpoint is changed to empty, should emit the unmounted signal
}

DFMBlockMonitor::DFMBlockMonitor(QObject *parent)
    : DFMMonitor (new DFMBlockMonitorPrivate(this), parent)
{
    auto subd = castSubPrivate<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    registerStartMonitor(std::bind(&DFMBlockMonitorPrivate::startMonitor, subd));
    registerStopMonitor(std::bind(&DFMBlockMonitorPrivate::stopMonitor, subd));
    registerStatus(std::bind(&DFMBlockMonitorPrivate::status, subd));
    registerMonitorObjectType(std::bind(&DFMBlockMonitorPrivate::monitorObjectType, subd));
    registerGetDevices(std::bind(&DFMBlockMonitorPrivate::getDevices, subd));
}

DFMBlockMonitor::~DFMBlockMonitor()
{
    auto subd = castSubPrivate<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    subd->stopMonitor();
}

