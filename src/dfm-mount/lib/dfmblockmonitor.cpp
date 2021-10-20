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
#include "base/dfmdevice.h"
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
            UDisksEncrypted *encrypted = udisks_object_peek_encrypted(blockObject);

            if (block) {
                DFMBlockDevice *dev = nullptr;
                // if already contained this block, update handlers.
                if (this->devices.contains(blkObjPath))
                    dev = this->devices[blkObjPath];
                else
                    dev = new DFMBlockDevice(q);

                auto blkDp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(dev->d.data());
                blkDp->blockHandler = block;
                blkDp->blkObjPath = blkObjPath;

                if (filesystem)
                    blkDp->fileSystemHandler = filesystem;

                if (encrypted)
                    blkDp->encryptedHandler = encrypted;

                if (partition)
                    blkDp->partitionHandler = partition;

                char *driveObjPath = udisks_block_dup_drive(block);
                UDisksObject *driveObject = udisks_client_peek_object(client, driveObjPath);

                if (driveObject) {
                    UDisksDrive *drive = udisks_object_peek_drive(driveObject);
                    if (drive) {
                        blkDp->driveHandler = drive;
                        this->drives.insert(QString(driveObjPath), drive);
                        auto &devsOfDrive = this->devicesOfDrive[driveObjPath];
                        if (!devsOfDrive.contains(dev))
                            devsOfDrive.append(dev);
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
    auto q = d->q;

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
         Q_EMIT q->driveAdded(objKey);
    }

    UDisksBlock *block = udisks_object_peek_block(udisksObj);
    UDisksFilesystem *fileSystem = udisks_object_peek_filesystem(udisksObj);
    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);
    UDisksEncrypted *encrypted = udisks_object_peek_encrypted(udisksObj);

    DFMBlockDevice *blkDev = nullptr;
    if (block) {
        blkDev = new DFMBlockDevice(d->q);
        auto blkDp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(blkDev->d.data());
        blkDp->blockHandler = block;
        blkDp->blkObjPath = objKey;

        QString drive = blkDev->getProperty(Property::BlockDrive).toString();
        if (d->drives.contains(drive)) {
            blkDp->driveHandler = d->drives.value(drive);
            d->devicesOfDrive[drive].append(blkDev);
        }

        if (fileSystem) {
           blkDp->fileSystemHandler = fileSystem;
            // Q_EMIT monitor->filesystemadded
        }
        if (partition) {
            blkDp->partitionHandler = partition;
            // Q_EMIT monitor->partitionadded
        }
        if (encrypted) {
            blkDp->encryptedHandler = encrypted;
        }

        Q_EMIT q->deviceAdded(blkDev);
        d->devices.insert(objKey, blkDev);
    }
}

void DFMBlockMonitorPrivate::onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);

    DFMBlockMonitorPrivate *d = static_cast<DFMBlockMonitorPrivate *>(userData);
    Q_ASSERT_X(d, __FUNCTION__, "monitor is not valid");
    auto q = d->q;

    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;

    // in past time, we never care about the UDisksJob, so we ignore it here.
    UDisksJob *job = udisks_object_peek_job(udisksObj);
    if (job)
        return;

    QString objKey = g_dbus_object_get_object_path(obj);
    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    if (drive) {
        qDebug() << "driveRemoved";
        d->drives.remove(objKey);
        d->devicesOfDrive.remove(objKey);
        Q_EMIT q->driveRemoved(objKey);
    }

    UDisksBlock *block = udisks_object_peek_block(udisksObj);
    if (block) {
        // we get the handler by *_PEEK_* funcs, so there is no need to release it by ourselves.
        auto *dev = d->devices.take(objKey);
        delete dev;
        Q_EMIT q->deviceRemoved(objKey);

        auto driveObjPath = udisks_block_get_drive(block);
        if (d->devicesOfDrive.contains(driveObjPath)) {
            d->devicesOfDrive[driveObjPath].removeAll(dev);
            if (d->devicesOfDrive.value(driveObjPath).isEmpty())
                d->devicesOfDrive.remove(driveObjPath);
        }
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
    auto q = d->q;

    // obtain the object path like "/org/freedesktop/UDisks2/block_devices/sdb1" from dbusProxy
    // and which is the key of DFMBlockMonitorPrivate::devices
    QString objPath = dbusProxy ? QString(g_dbus_proxy_get_object_path(dbusProxy)) : QString();
    if (objPath.isEmpty())
        return;

    qDebug() << objPath << "; " << __FUNCTION__;

    // obtain the DFMDevice(s) handler to emit the propertyChanged signal
    QList<DFMBlockDevice *> devs;
    if (objPath.startsWith("/org/freedesktop/UDisks2/drives/")) { // means this is a DRIVE objech which property has changed
        qDebug() << "drive changed...";
        // so we can notify all of the block devices of this drive that the dirve's property has changed.
        devs = d->devicesOfDrive.value(objPath);
    } else { // we treat it as Blocks
        DFMBlockDevice *dev = d->devices.value(objPath, nullptr);
        if (dev)
            devs << dev;
    }
    if (devs.isEmpty()) { // since there is no device for this object, so there is no need to report changes
        qDebug() << "no devies to notify.";
        return;
    }

    GVariantIter *iter = nullptr;
    const char *property_name = nullptr;
    GVariant *value = nullptr;
    QMap<Property, QVariant> changes;
    g_variant_get (property, "a{sv}", &iter);
    while (g_variant_iter_next(iter, "{&sv}", &property_name, &value)) {
        char *value_str = g_variant_print(value, false);
        QVariant newVal = gvariantToQVariant(value);

        qDebug() << property_name << "\t\tchanged, the new value is: " << value_str
                 << "\nconverted value is: " << newVal;

        Property property = propertyName2Property.value(property_name, Property::BlockProperty);
        if (property == Property::BlockProperty) {
            qDebug() << "cannot find the enum of " << property_name;
            continue;
        }

        changes.insert(property, newVal);
    }

    // in most cases, the item count of devs will be 1, only when drive property changes, the counts might bigger than 1.
    for (auto iter = devs.cbegin(); iter != devs.cend(); iter += 1) {
        auto *dev = (*iter);
        if (dev) {
            Q_EMIT dev->propertyChanged(changes);
            Q_EMIT q->propertyChanged(dev, changes);
        }

        // if the mountpoints changed to empty, then we think it is unmounted
        if (changes.contains(Property::FileSystemMountPoint)) {
            auto mpts = changes.value(Property::FileSystemMountPoint).toStringList();
            if (mpts.isEmpty()) {
                Q_EMIT dev->unmounted();
                Q_EMIT q->mountRemoved(dev);
            } else {
                Q_EMIT dev->mounted(mpts.first());
                Q_EMIT q->mountAdded(dev, mpts.first());
            }
        }
    }
}

DFMBlockMonitor::DFMBlockMonitor(QObject *parent)
    : DFMMonitor (new DFMBlockMonitorPrivate(this), parent)
{
    auto dp = castSubPrivate<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    registerStartMonitor(std::bind(&DFMBlockMonitorPrivate::startMonitor, dp));
    registerStopMonitor(std::bind(&DFMBlockMonitorPrivate::stopMonitor, dp));
    registerStatus(std::bind(&DFMBlockMonitorPrivate::status, dp));
    registerMonitorObjectType(std::bind(&DFMBlockMonitorPrivate::monitorObjectType, dp));
    registerGetDevices(std::bind(&DFMBlockMonitorPrivate::getDevices, dp));
}

DFMBlockMonitor::~DFMBlockMonitor()
{
    auto dp = castSubPrivate<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    dp->stopMonitor();
}

#define StringPropertyItem(key, val)    std::pair<QString, Property>(key, val)

const QMap<QString, Property> DFMBlockMonitorPrivate::propertyName2Property {
    StringPropertyItem("Configuration",           Property::BlockConfiguration),
    StringPropertyItem("CryptoBackingDevice",     Property::BlockCryptoBackingDevice),
    StringPropertyItem("Device",                  Property::BlockDevice),
    StringPropertyItem("Drive",                   Property::BlockDrive),
    StringPropertyItem("IdLabel",                 Property::BlockIDLabel),
    StringPropertyItem("IdType",                  Property::BlockIDType),
    StringPropertyItem("IdUsage",                 Property::BlockIDUsage),
    StringPropertyItem("IdUUID",                  Property::BlockIDUUID),
    StringPropertyItem("IdVersion",               Property::BlockIDVersion),
    StringPropertyItem("DeviceNumber",            Property::BlockDeviceNumber),
    StringPropertyItem("PreferredDevice",         Property::BlockPreferredDevice),
    StringPropertyItem("Id",                      Property::BlockID),
    StringPropertyItem("Size",                    Property::BlockSize),
    StringPropertyItem("ReadOnly",                Property::BlockReadOnly),
    StringPropertyItem("Symlinks",                Property::BlockSymlinks),
    StringPropertyItem("HintPartitionable",       Property::BlockHintPartitionable),
    StringPropertyItem("HintSystem",              Property::BlockHintSystem),
    StringPropertyItem("HintIgnore",              Property::BlockHintIgnore),
    StringPropertyItem("HintAuto",                Property::BlockHintAuto),
    StringPropertyItem("HintName",                Property::BlockHintName),
    StringPropertyItem("HintIconName",            Property::BlockHintIconName),
    StringPropertyItem("HintSymbolicIconName",    Property::BlockHintSymbolicIconName),
    StringPropertyItem("MdRaid",                  Property::BlockMdRaid),
    StringPropertyItem("MdRaidMember",            Property::BlockMdRaidMember),
    StringPropertyItem("ConnectionBus",           Property::DriveConnectionBus),
    StringPropertyItem("Removable",               Property::DriveRemovable),
    StringPropertyItem("Ejectable",               Property::DriveEjectable),
    StringPropertyItem("Seat",                    Property::DriveSeat),
    StringPropertyItem("Media",                   Property::DriveMedia),
    StringPropertyItem("MediaCompatibility",      Property::DriveMediaCompatibility),
    StringPropertyItem("MediaRemovable",          Property::DriveMediaRemovable),
    StringPropertyItem("MediaAvailable",          Property::DriveMediaAvailable),
    StringPropertyItem("MediaChangeDetected",     Property::DriveMediaChangeDetected),
    StringPropertyItem("TimeDetected",            Property::DriveTimeDetected),
    StringPropertyItem("TimeMediaDetected",       Property::DriveTimeMediaDetected),
    StringPropertyItem("Size",                    Property::DriveSize),
    StringPropertyItem("Optical",                 Property::DriveOptical),
    StringPropertyItem("OpticalBlank",            Property::DriveOpticalBlank),
    StringPropertyItem("OpticalNumTracks",        Property::DriveOpticalNumTracks),
    StringPropertyItem("OpticalNumAudioTracks",   Property::DriveOpticalNumAudioTracks),
    StringPropertyItem("OpticalNumDataTracks",    Property::DriveOpticalNumDataTracks),
    StringPropertyItem("OpticalNumSessions",      Property::DriveOpticalNumSessions),
    StringPropertyItem("Model",                   Property::DriveModel),
    StringPropertyItem("Revision",                Property::DriveRevision),
    StringPropertyItem("RotationRate",            Property::DriveRotationRate),
    StringPropertyItem("Serial",                  Property::DriveSerial),
    StringPropertyItem("Vender",                  Property::DriveVender),
    StringPropertyItem("WWN",                     Property::DriveWWN),
    StringPropertyItem("SortKey",                 Property::DriveSortKey),
    StringPropertyItem("Configuration",           Property::DriveConfiguration),
    StringPropertyItem("ID",                      Property::DriveID),
    StringPropertyItem("CanPowerOff",             Property::DriveCanPowerOff),
    StringPropertyItem("SiblingID",               Property::DriveSiblingID),
    StringPropertyItem("MountPoints",             Property::FileSystemMountPoint),
    StringPropertyItem("Number",                  Property::PartitionNumber),
    StringPropertyItem("Type",                    Property::PartitionType),
    StringPropertyItem("Offset",                  Property::PartitionOffset),
    StringPropertyItem("Size",                    Property::PartitionSize),
    StringPropertyItem("Flags",                   Property::PartitionFlags),
    StringPropertyItem("Name",                    Property::PartitionName),
    StringPropertyItem("UUID",                    Property::PartitionUUID),
    StringPropertyItem("Table",                   Property::PartitionTable),
    StringPropertyItem("IsContainer",             Property::PartitionIsContainer),
    StringPropertyItem("IsContained",             Property::PartitionIsContained),
};
