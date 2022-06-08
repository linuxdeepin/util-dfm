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

#include "base/ddevice.h"
#include "base/dmount_global.h"
#include "base/dmountutils.h"
#include "dblockdevice.h"
#include "dblockmonitor.h"
#include "private/dblockdevice_p.h"
#include "private/dblockmonitor_p.h"

#include <QDebug>
#include <QMapIterator>
#include <QMap>
#include <QTime>
#include <QTimer>

DFM_MOUNT_USE_NS

#define UDISKS_BLOCK_PATH_PREFIX "/org/freedesktop/UDisks2/block_devices/"
#define UDISKS_DRIVE_PATH_PREFIX "/org/freedesktop/UDisks2/drives/"
#define UDISKS_BLOCK_IFACE_FILESYSTEM "org.freedesktop.UDisks2.Filesystem"
#define UDISKS_BLOCK_IFACE_PARTITIONTABLE "org.freedesktop.UDisks2.PartitionTable"

QMap<QString, QSet<QString>> DBlockMonitorPrivate::blksOfDrive = {};

DBlockMonitorPrivate::DBlockMonitorPrivate(DBlockMonitor *qq)
    : DDeviceMonitorPrivate(qq)
{
    GError *err = nullptr;
    client = udisks_client_new_sync(nullptr, &err);
    if (err) {
        qCritical() << "init udisks client failed. " << err->message;
        g_error_free(err);
    }

    initDevices();
}

DBlockMonitorPrivate::~DBlockMonitorPrivate()
{
    qDebug() << "block monitor release...";
    if (client) {
        g_object_unref(client);
        client = nullptr;
    }
}

bool DBlockMonitorPrivate::startMonitor()
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

    auto handler = g_signal_connect(dbusMng, OBJECT_ADDED, G_CALLBACK(&DBlockMonitorPrivate::onObjectAdded), q);
    connections.insert(OBJECT_ADDED, handler);

    handler = g_signal_connect(dbusMng, OBJECT_REMOVED, G_CALLBACK(&DBlockMonitorPrivate::onObjectRemoved), q);
    connections.insert(OBJECT_REMOVED, handler);

    handler = g_signal_connect(dbusMng, PROPERTY_CHANGED, G_CALLBACK(&DBlockMonitorPrivate::onPropertyChanged), q);
    connections.insert(PROPERTY_CHANGED, handler);

    handler = g_signal_connect(dbusMng, INTERFACE_ADDED, G_CALLBACK(&DBlockMonitorPrivate::onInterfaceAdded), q);
    connections.insert(INTERFACE_ADDED, handler);

    handler = g_signal_connect(dbusMng, INTERFACE_REMOVED, G_CALLBACK(&DBlockMonitorPrivate::onInterfaceRemoved), q);
    connections.insert(INTERFACE_REMOVED, handler);

    qDebug() << "block monitor start";
    return true;
}

bool DBlockMonitorPrivate::stopMonitor()
{
    if (!client) {
        qDebug() << "client is not valid";
        return false;
    }

    GDBusObjectManager *dbusMng = udisks_client_get_object_manager(client);
    for (auto iter = connections.cbegin(); iter != connections.cend(); iter++)
        g_signal_handler_disconnect(dbusMng, iter.value());
    connections.clear();

    qDebug() << "block monitor stop";
    return true;
}

DeviceType DBlockMonitorPrivate::monitorObjectType() const
{
    return DeviceType::kBlockDevice;
}

QStringList DBlockMonitorPrivate::getDevices()
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

QSharedPointer<DDevice> DBlockMonitorPrivate::createDeviceById(const QString &id)
{
    auto blk = new DBlockDevice(client, id, nullptr);
    // for a block device, there must have a block node in dbus, otherwise treat it as a invalid object
    if (blk->hasBlock()) {
        QSharedPointer<DDevice> ret;
        ret.reset(blk);
        return ret;
    } else {
        delete blk;
        blk = nullptr;
        return QSharedPointer<DDevice>(nullptr);
    }
}

QStringList DBlockMonitorPrivate::resolveDevice(const QVariantMap &devspec, const QVariantMap &opts)
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

QStringList DBlockMonitorPrivate::resolveDeviceNode(const QString &node, const QVariantMap &opts)
{
    if (node.isEmpty())
        return QStringList();
    QVariantMap devSpec { { "path", node } };
    return resolveDevice(devSpec, opts);
}

QStringList DBlockMonitorPrivate::resolveDeviceOfDrive(const QString &drvObjPath)
{
    if (q->status() != MonitorStatus::kMonitoring)
        initDevices();
    return blksOfDrive.value(drvObjPath).toList();
}

void DBlockMonitorPrivate::onObjectAdded(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);
    DBlockMonitor *q = static_cast<DBlockMonitor *>(userData);
    Q_ASSERT(q);

    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;
    QString objPath = g_dbus_object_get_object_path(obj);

    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    UDisksBlock *block = udisks_object_peek_block(udisksObj);
    UDisksFilesystem *fileSystem = udisks_object_peek_filesystem(udisksObj);
    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);
    UDisksEncrypted *encrypted = udisks_object_peek_encrypted(udisksObj);

    if (drive) {
        qDebug() << "drive added: " << objPath;
        Q_EMIT q->driveAdded(objPath);

        blksOfDrive.insert(objPath, {});
    }
    if (block) {
        qDebug() << "block added: " << objPath;
        Q_EMIT q->deviceAdded(objPath);

        QString drvPath(udisks_block_get_drive(block));
        blksOfDrive[drvPath].insert(objPath);

        g_autofree char *encryptedShell = udisks_block_dup_crypto_backing_device(block);
        if (strcmp(encryptedShell, "/") != 0) {
            Q_EMIT q->blockUnlocked(encryptedShell, objPath);
            qDebug() << "unlocked: " << encryptedShell << "-->" << objPath;
        }
    }
    if (fileSystem) {
        qDebug() << "filesystem added: " << objPath << ", filesystem: " << fileSystem;
        Q_EMIT q->fileSystemAdded(objPath);
    }
    if (partition) {
        qDebug() << "partition added: " << objPath;
    }
    if (encrypted) {
        qDebug() << "encrypted added: " << objPath;
    }
}

void DBlockMonitorPrivate::onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);

    DBlockMonitor *q = static_cast<DBlockMonitor *>(userData);
    Q_ASSERT(q);

    UDisksObject *udisksObj = UDISKS_OBJECT(obj);
    if (!udisksObj)
        return;

    QString objPath = g_dbus_object_get_object_path(obj);

    UDisksDrive *drive = udisks_object_peek_drive(udisksObj);
    UDisksBlock *block = udisks_object_peek_block(udisksObj);
    UDisksFilesystem *fileSystem = udisks_object_peek_filesystem(udisksObj);
    UDisksPartition *partition = udisks_object_peek_partition(udisksObj);
    UDisksEncrypted *encrypted = udisks_object_peek_encrypted(udisksObj);

    if (drive) {
        qDebug() << "drive removed: " << objPath;
        Q_EMIT q->driveRemoved(objPath);

        blksOfDrive.remove(objPath);
    }
    if (block) {
        qDebug() << "block removed: " << objPath;
        Q_EMIT q->deviceRemoved(objPath);

        QString drvPath(udisks_block_get_drive(block));
        if (blksOfDrive.contains(QString(drvPath)))
            blksOfDrive[drvPath].remove(objPath);

        g_autofree char *encryptedShell = udisks_block_dup_crypto_backing_device(block);
        if (strcmp(encryptedShell, "/") != 0) {
            Q_EMIT q->blockLocked(encryptedShell);
            qDebug() << "locked: " << objPath << "removed, " << encryptedShell << "locked";
        }
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

void DBlockMonitorPrivate::onPropertyChanged(GDBusObjectManagerClient *mngClient, GDBusObjectProxy *dbusObjProxy, GDBusProxy *dbusProxy,
                                             GVariant *property, const gchar *const invalidProperty, gpointer userData)
{
    Q_UNUSED(mngClient);
    Q_UNUSED(dbusObjProxy);
    Q_UNUSED(invalidProperty);

    DBlockMonitor *q = static_cast<DBlockMonitor *>(userData);
    if (!q)
        return;

    // get the changed object path: "/org/freedesktop/UDisks2/block_devices/sdb1"
    QString objPath = dbusProxy ? QString(g_dbus_proxy_get_object_path(dbusProxy)) : QString();
    QString ifaceName = dbusProxy ? QString(g_dbus_proxy_get_interface_name(dbusProxy)) : QString();
    bool isBlockChanged = objPath.startsWith(UDISKS_BLOCK_PATH_PREFIX);
    bool isDriveChanged = objPath.startsWith(UDISKS_DRIVE_PATH_PREFIX);
    if (!isBlockChanged && !isDriveChanged) return;

    QMap<Property, QVariant> changes;
    QVariant val = Utils::castFromGVariant(property);
    if (val.type() == QVariant::Map) {
        QVariantMap vmap = val.toMap();
        for (auto iter = vmap.cbegin(); iter != vmap.cend(); ++iter) {
            auto key = iter.key();
            auto val = iter.value();
            Property type = Utils::getPropertyByName(key, ifaceName);
            if (type == Property::kNotInit) {
                qDebug() << "\tproperty: " << key << "has no mapped type, but value is" << val;
                continue;
            }
            changes.insert(type, val);
        }
    } else {
        qWarning() << "property is not dict" << val;
        return;
    }

    if (changes.isEmpty())
        return;

    if (changes.contains(Property::kFileSystemMountPoint)) {
        auto mpts = changes.value(Property::kFileSystemMountPoint).toStringList();
        // make a short delay to emit (un)mounted signal, in case that at the moment device handler is created, the property is not ready.
        QTimer::singleShot(100, q, [=] { Q_EMIT mpts.isEmpty() ? q->mountRemoved(objPath) : q->mountAdded(objPath, mpts.first()); });
    }

    if (isBlockChanged) {
        Q_EMIT q->propertyChanged(objPath, changes);
    } else {
        QSet<QString> blks = blksOfDrive.value(objPath);
        for (const auto &blk : blks)
            Q_EMIT q->propertyChanged(blk, changes);
    }
}

void DBlockMonitorPrivate::onInterfaceAdded(GDBusObjectManager *mng, GDBusObject *obj, GDBusInterface *iface, gpointer userData)
{
    DBlockMonitor *q = static_cast<DBlockMonitor *>(userData);
    Q_ASSERT(q);

    QString objPath = g_dbus_object_get_object_path(obj);
    if (!objPath.startsWith(UDISKS_BLOCK_PATH_PREFIX))
        return;
    auto info = g_dbus_interface_get_info(iface);
    if (strcmp(info->name, UDISKS_BLOCK_IFACE_FILESYSTEM) == 0) {
        qDebug() << "filesystem added: " << objPath;
        Q_EMIT q->fileSystemAdded(objPath);
    } /*else if (strcmp(info->name, UDISKS_BLOCK_IFACE_PARTITIONTABLE) == 0) {
    }*/
}

void DBlockMonitorPrivate::onInterfaceRemoved(GDBusObjectManager *mng, GDBusObject *obj, GDBusInterface *iface, gpointer userData)
{
    DBlockMonitor *q = static_cast<DBlockMonitor *>(userData);
    Q_ASSERT(q);

    QString objPath = g_dbus_object_get_object_path(obj);
    if (!objPath.startsWith(UDISKS_BLOCK_PATH_PREFIX))
        return;
    auto info = g_dbus_interface_get_info(iface);
    if (strcmp(info->name, UDISKS_BLOCK_IFACE_FILESYSTEM) == 0) {
        qDebug() << "filesystem removed: " << objPath;
        Q_EMIT q->fileSystemRemoved(objPath);
    } /*else if (strcmp(info->name, UDISKS_BLOCK_IFACE_PARTITIONTABLE) == 0) {
    }*/
}

void DBlockMonitorPrivate::initDevices()
{
    blksOfDrive.clear();
    auto lst = getDevices();
    for (const auto &blk : lst) {
        std::string str = blk.toStdString();
        UDisksObject *obj = udisks_client_peek_object(client, str.c_str());
        if (!obj)
            continue;

        auto blkObj = udisks_object_peek_block(obj);
        if (!blkObj)
            continue;

        const char *drive = udisks_block_get_drive(blkObj);
        if (drive)
            blksOfDrive[QString(drive)].insert(blk);
    }
}

DBlockMonitor::DBlockMonitor(QObject *parent)
    : DDeviceMonitor(new DBlockMonitorPrivate(this), parent)
{
    auto dp = Utils::castClassFromTo<DDeviceMonitorPrivate, DBlockMonitorPrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    registerStartMonitor(std::bind(&DBlockMonitorPrivate::startMonitor, dp));
    registerStopMonitor(std::bind(&DBlockMonitorPrivate::stopMonitor, dp));
    registerMonitorObjectType(std::bind(&DBlockMonitorPrivate::monitorObjectType, dp));
    registerGetDevices(std::bind(&DBlockMonitorPrivate::getDevices, dp));
    registerCreateDeviceById(std::bind(&DBlockMonitorPrivate::createDeviceById, dp, std::placeholders::_1));
}

DBlockMonitor::~DBlockMonitor()
{
    auto dp = Utils::castClassFromTo<DDeviceMonitorPrivate, DBlockMonitorPrivate>(d.data());
    if (dp)
        dp->stopMonitor();
}

/*!
 * \brief DBlockMonitor::resolveDevice
 * \param devspec:  currently support keys: path, label and uuid
 * \param opts:     currently unused in udisks2
 * \return          the associated device object paths
 */
QStringList DBlockMonitor::resolveDevice(const QVariantMap &devspec, const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DDeviceMonitorPrivate, DBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDevice(devspec, opts) : QStringList();
}

QStringList DBlockMonitor::resolveDeviceNode(const QString &node, const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DDeviceMonitorPrivate, DBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDeviceNode(node, opts) : QStringList();
}

/*!
 * \brief DBlockMonitor::resolveDeviceFromDrive
 * \param drvObjPath:   object path for drive
 * \return              the associated block object paths
 */
QStringList DBlockMonitor::resolveDeviceFromDrive(const QString &drvObjPath)
{
    auto dp = Utils::castClassFromTo<DDeviceMonitorPrivate, DBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDeviceOfDrive(drvObjPath) : QStringList();
}
