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
#include "base/dfmmountutils.h"

#include <QDebug>
#include <QMapIterator>
#include <QMap>

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

QStringList DFMBlockMonitorPrivate::getDevices()
{
    Q_ASSERT(client);
    UDisksManager *mng = udisks_client_get_manager(client);
    Q_ASSERT(mng);

    GVariant *gopts = Utils::castFromQVariantMap({});
    char **results = nullptr;
    GError *err = nullptr;
    bool ret = udisks_manager_call_get_block_devices_sync(mng, gopts, &results, nullptr, &err);
    if (ret) {
        QStringList blks;
        int next = 0;
        while (results && results[next]) {
            blks << QString(results[next]);
            next++;
        }

        if (results)
            g_strfreev(results);
        return blks;
    } else {
        // TODO: ERROR HANDLE
        if (err)
            g_error_free(err);
    }
    return QStringList();
}

QSharedPointer<DFMDevice> DFMBlockMonitorPrivate::createDeviceById(const QString &id)
{
    if (!getDevices().contains(id))
        return nullptr;
    return QSharedPointer<DFMDevice>(new DFMBlockDevice(client, id, q));
}

QStringList DFMBlockMonitorPrivate::resolveDevice(const QVariantMap &devspec, const QVariantMap &opts)
{
    Q_ASSERT(client);

    char **results = nullptr;
    GError *err = nullptr;
    UDisksManager *mng = udisks_client_get_manager(client);
    GVariant *gDevSpec = Utils::castFromQVariantMap(devspec);
    GVariant *GOpts = Utils::castFromQVariantMap(opts);
    bool ok = udisks_manager_call_resolve_device_sync(mng, gDevSpec, GOpts, &results, nullptr, &err);
    if (ok) {
        QStringList ret;
        int next = 0;
        while (results && results[next]) {
            ret << QString(results[next]);
            next++;
        }

        if (results)
            g_strfreev(results);
        return ret;
    } else {
        // TODO: ERROR HANDLE
        if (err)
            g_error_free(err);
    }
    return {};
}

QStringList DFMBlockMonitorPrivate::resolveDeviceNode(const QString &node, const QVariantMap &opts)
{
    if (node.isEmpty())
        return QStringList();
    QVariantMap devSpec {{"path", node}};
    return resolveDevice(devSpec, opts);
}

QStringList DFMBlockMonitorPrivate::resolveDeviceOfDrive(const QString &drvObjPath)
{
    // TODO: COMPLETE IT
    return QStringList();
}

void DFMBlockMonitorPrivate::onObjectAdded(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);
    DFMBlockMonitorPrivate *d = static_cast<DFMBlockMonitorPrivate *>(userData);
    Q_ASSERT(d);

    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;

    QString objPath = g_dbus_object_get_object_path(obj);

    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    UDisksBlock *block = udisks_object_peek_block(udisksObj);
    UDisksFilesystem *fileSystem = udisks_object_peek_filesystem(udisksObj);
    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);
    UDisksEncrypted *encrypted = udisks_object_peek_encrypted(udisksObj);

    auto q = castClassFromTo<DFMMonitor, DFMBlockMonitor>(d->q);
    if (!q)
        return;

    if (drive) {
        qDebug() << "drive added: " << objPath;
        Q_EMIT q->driveAdded(objPath);
    }
    if (block) {
        qDebug() << "block added: " << objPath;
        Q_EMIT q->deviceAdded(objPath);
    }
    if (fileSystem) {
        qDebug() << "filesystem added: " << objPath;
        Q_EMIT q->fileSystemAdded(objPath);
    }
    if (partition) {
        qDebug() << "partition added: " << objPath;
    }
    if (encrypted) {
        qDebug() << "encrypted added: " << objPath;
    }
}

void DFMBlockMonitorPrivate::onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);

    DFMBlockMonitorPrivate *d = static_cast<DFMBlockMonitorPrivate *>(userData);
    Q_ASSERT(d);

    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;

    QString objPath = g_dbus_object_get_object_path(obj);

    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    UDisksBlock *block = udisks_object_peek_block(udisksObj);
    UDisksFilesystem *fileSystem = udisks_object_peek_filesystem(udisksObj);
    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);
    UDisksEncrypted *encrypted = udisks_object_peek_encrypted(udisksObj);

    auto q = castClassFromTo<DFMMonitor, DFMBlockMonitor>(d->q);
    if (!q)
        return;
    if (drive) {
        qDebug() << "drive removed: " << objPath;
        Q_EMIT q->driveRemoved(objPath);
    }
    if (block) {
        qDebug() << "block removed: " << objPath;
        Q_EMIT q->deviceRemoved(objPath);
    }
    if (fileSystem) {
        qDebug() << "filesystem removed: " << objPath;
        Q_EMIT q->fileSystemRemoved(objPath);
    }
    if (partition) {
        qDebug() << "partition removed: " << objPath;
    }
    if (encrypted) {
        qDebug() << "encrypted removed: " << objPath;
    }
}

void DFMBlockMonitorPrivate::onPropertyChanged(GDBusObjectManagerClient *mngClient, GDBusObjectProxy *dbusObjProxy, GDBusProxy *dbusProxy,
                                               GVariant *property, const gchar * const invalidProperty, gpointer userData)
{
    Q_UNUSED(mngClient);
    Q_UNUSED(dbusObjProxy);
    Q_UNUSED(invalidProperty);

    DFMBlockMonitorPrivate *d = static_cast<DFMBlockMonitorPrivate *>(userData);
    Q_ASSERT(d);

    // get the changed object path: "/org/freedesktop/UDisks2/block_devices/sdb1"
    QString objPath = dbusProxy ? QString(g_dbus_proxy_get_object_path(dbusProxy)) : QString();
    bool isBlockChanged = objPath.startsWith("/org/freedesktop/UDisks2/block_devices/");
    bool isDriveChanged = objPath.startsWith("/org/freedesktop/UDisks2/drives/");
    if (!isBlockChanged && !isDriveChanged) return;

    qDebug() << objPath << "property changed";
    qDebug() << "\t\t" << g_variant_print(property, false);
    QMap<Property, QVariant> changes;
    QVariant val = Utils::castFromGVariant(property);
    if (val.type() == QVariant::Map) {
        QVariantMap vmap = val.toMap();
        auto iter = vmap.cbegin();
        while (iter != vmap.cend()) {
            auto key = iter.key();
            auto val = iter.value();
            iter++;
            Property type = propertyName2Property.value(key, Property::NotInit);
            if (type == Property::NotInit) {
                qDebug() << "\tproperty: " << key << "has no mapped type, but value is" << val;
                continue;
            }

            qDebug() << "\tproperty: " << key << "changed to " << val;
            changes.insert(type, val);
        }
    } else {
        qDebug() << "property is not dict" << val;
        return;
    }

    if (changes.isEmpty()) {
        qDebug() << "\tno import changes to report";
        return;
    }

    auto q = castClassFromTo<DFMMonitor, DFMBlockMonitor>(d->q);
    if (!q) return;

    if (changes.contains(Property::FileSystemMountPoint)) {
        auto mpts = changes.value(Property::FileSystemMountPoint).toStringList();
        Q_EMIT mpts.isEmpty() ? q->mountRemoved(objPath) : q->mountAdded(objPath, mpts.first());
    }
    if (changes.contains(Property::BlockIDLabel)) {
        ; // emit idLabel changed
    }
    Q_EMIT q->propertyChanged(objPath, changes);
}

DFMBlockMonitor::DFMBlockMonitor(QObject *parent)
    : DFMMonitor (new DFMBlockMonitorPrivate(this), parent)
{
    auto dp = castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    registerStartMonitor(std::bind(&DFMBlockMonitorPrivate::startMonitor, dp));
    registerStopMonitor(std::bind(&DFMBlockMonitorPrivate::stopMonitor, dp));
    registerStatus(std::bind(&DFMBlockMonitorPrivate::status, dp));
    registerMonitorObjectType(std::bind(&DFMBlockMonitorPrivate::monitorObjectType, dp));
    registerGetDevices(std::bind(&DFMBlockMonitorPrivate::getDevices, dp));
    registerCreateDeviceById(std::bind(&DFMBlockMonitorPrivate::createDeviceById, dp, std::placeholders::_1));
}

DFMBlockMonitor::~DFMBlockMonitor()
{
    auto dp = castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    if (dp)
        dp->stopMonitor();
}

/*!
 * \brief DFMBlockMonitor::resolveDevice
 * \param devspec:  currently support keys: path, label and uuid
 * \param opts:     currently unused in udisks2
 * \return          the associated device object paths
 */
QStringList DFMBlockMonitor::resolveDevice(const QVariantMap &devspec, const QVariantMap &opts)
{
    auto dp = castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDevice(devspec, opts) : QStringList();
}

QStringList DFMBlockMonitor::resolveDeviceNode(const QString &node, const QVariantMap &opts)
{
    auto dp = castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDeviceNode(node, opts) : QStringList();
}

/*!
 * \brief DFMBlockMonitor::resolveDeviceFromDrive
 * \param drvObjPath:   object path for drive
 * \return              the associated block object paths
 */
QStringList DFMBlockMonitor::resolveDeviceFromDrive(const QString &drvObjPath)
{
    auto dp = castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDeviceOfDrive(drvObjPath) : QStringList();
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
