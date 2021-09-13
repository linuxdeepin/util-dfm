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

    getDevicesOnInit();

    // the mng is owned by client, do not free it manually
    GDBusObjectManager *dbusMng = udisks_client_get_object_manager(client);
    if (!dbusMng) {
        g_object_unref(client);
        client = nullptr;
        qCritical() << "start monitor block failed: cannot get dbus monitor";
        return false;
    }

    // there is no good way to solve these warnings... but it does not affect anything.
    g_signal_connect(dbusMng, "object-added", G_CALLBACK(&DFMBlockMonitorPrivate::onObjectAdded), this);
    g_signal_connect(dbusMng, "object_removed", G_CALLBACK(&DFMBlockMonitorPrivate::onObjectRemoved), this);
    g_signal_connect(dbusMng, "interface-proxy-properties-changed", G_CALLBACK(&DFMBlockMonitorPrivate::onPropertyChanged), this);

    curStatus = MonitorStatus::Monitoring;

    qDebug() << "monitor started...";
    return true;
}

bool DFMBlockMonitorPrivate::stopMonitor()
{
    if (client) {
        g_object_unref(client);
        client = nullptr;
    }

    // clear the caches and release the mems.
    QStringList keys = devices.keys();
    for (int i = 0; i < keys.count(); i++) {
        auto *dev = devices.take(keys.at(i));
        delete dev;
        dev = nullptr;
    }
    devices.clear();

    // the values of drivers which we use *_PEEK_* to obtain them belong to UDisksClient, they are just refs, do not free/unref it.
    // which is declared on the API page of UDisksObject:
    // http://storaged.org/doc/udisks2-api/latest/UDisksObject.html#udisks-object-peek-drive
    drives.clear();
    // the values of devicesOfDrive are just a ref of DFMBlockDevice, which has been released above, so do not free it too.
    devicesOfDrive.clear();

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

QList<DFMDevice *> DFMBlockMonitorPrivate::getDevices() const
{
    QList<DFMDevice *> ret;
    QMapIterator<QString, DFMBlockDevice *> iter(devices);
    while (iter.hasNext()) {
        iter.next();
        ret.push_back(iter.value());
    }
    return ret;
}

void DFMBlockMonitorPrivate::getDevicesOnInit()
{
    Q_ASSERT_X(client, __FUNCTION__, "client must be valid to get all devices");
    UDisksManager *mng = udisks_client_get_manager(client);
    Q_ASSERT_X(mng, __FUNCTION__, "manager must be valid");

    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manually.
    GVariant *gOpts = g_variant_builder_end(builder);

    char **blkObjs = nullptr;
    GError *err = nullptr;
    bool ok = udisks_manager_call_get_block_devices_sync(mng, gOpts, &blkObjs, nullptr, &err);
    if (ok && blkObjs) {
        int next = 0;
        while (blkObjs && blkObjs[next]) {
            UDisksObject *blockObject = udisks_client_peek_object(client, blkObjs[next]);
            if (!blockObject)
                continue;

            // TODO: I will remove all the debugs before it released.
            qDebug() << "********************start parsing block devices********************";
            qDebug() << "*\t" << blkObjs[next];
            UDisksBlock *block = udisks_object_peek_block(blockObject);
            UDisksFilesystem *filesystem = udisks_object_peek_filesystem(blockObject);
            UDisksPartition *partition = udisks_object_peek_partition(blockObject);

            if (block) {
                DFMBlockDevice *dev = new DFMBlockDevice(q);
                auto blkD = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(dev->d.data());
                blkD->blockHandler = block;
                qDebug() << "*\t\tblock founded";

                if (filesystem) {
                    blkD->fileSystemHandler = filesystem;
                    qDebug() << "*\t\tfilesystem founded";
                }

                if (partition) {
                    blkD->partitionHandler = partition;
                    qDebug() << "*\t\tpartition founded";
                }

                char *driveObjPath = udisks_block_dup_drive(block);
                UDisksObject *driveObject = udisks_client_peek_object(client, driveObjPath);

                if (driveObject) {
                    UDisksDrive *drive = udisks_object_peek_drive(driveObject);
                    if (drive) {
                        blkD->driveHandler = drive;
                        qDebug() << "*\t\tdrive founded";

                        this->drives.insert(QString(driveObjPath), drive);
                    }
                }

                this->devices.insert(blkObjs[next], dev);
                g_free(driveObjPath);

                qDebug() << "******************** end parsing block devices ********************\n";
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

    // in past time, we never care about the UDisksJob, so we ignore it here.
    UDisksJob *job = udisks_object_peek_job(udisksObj);
    if (job)
        return;
    qDebug() << "\t\t" << __FUNCTION__;

    QString objKey = g_dbus_object_get_object_path(obj);
    qDebug() << "\t" << objKey;

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

    qDebug() << "\t\t" << __FUNCTION__;

    QString objKey = g_dbus_object_get_object_path(obj);
    qDebug() << "\t" << objKey;

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

