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
#include <QTime>

DFM_MOUNT_USE_NS

#define UDISKS_BLOCK_PATH_PREFIX    "/org/freedesktop/UDisks2/block_devices/"
#define UDISKS_DRIVE_PATH_PREFIX    "/org/freedesktop/UDisks2/drives/"

QMap<QString, QSet<QString>> DFMBlockMonitorPrivate::blksOfDrive = {};

DFMBlockMonitorPrivate::DFMBlockMonitorPrivate(DFMBlockMonitor *qq)
    : DFMMonitorPrivate (qq)
{
    GError *err = nullptr;
    client = udisks_client_new_sync(nullptr, &err);
    if (err) {
        qCritical() << "init udisks client failed. " << err->message;
        g_error_free(err);
    }

    initDevices();
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

    auto handler = g_signal_connect(dbusMng, OBJECT_ADDED, G_CALLBACK(&DFMBlockMonitorPrivate::onObjectAdded), q);
    connections.insert(OBJECT_ADDED, handler);

    handler = g_signal_connect(dbusMng, OBJECT_REMOVED, G_CALLBACK(&DFMBlockMonitorPrivate::onObjectRemoved), q);
    connections.insert(OBJECT_REMOVED, handler);

    handler = g_signal_connect(dbusMng, PROPERTY_CHANGED, G_CALLBACK(&DFMBlockMonitorPrivate::onPropertyChanged), q);
    connections.insert(PROPERTY_CHANGED, handler);

    qDebug() << "block monitor start";
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

    qDebug() << "block monitor stop";
    return true;
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
    return blksOfDrive.value(drvObjPath).toList();
}

void DFMBlockMonitorPrivate::onObjectAdded(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);
    DFMBlockMonitor *q = static_cast<DFMBlockMonitor*>(userData);
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

void DFMBlockMonitorPrivate::onObjectRemoved(GDBusObjectManager *mng, GDBusObject *obj, gpointer userData)
{
    Q_UNUSED(mng);

    DFMBlockMonitor *q = static_cast<DFMBlockMonitor*>(userData);
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

    DFMBlockMonitor *q = static_cast<DFMBlockMonitor*>(userData);
    Q_ASSERT(q);

    // get the changed object path: "/org/freedesktop/UDisks2/block_devices/sdb1"
    QString objPath = dbusProxy ? QString(g_dbus_proxy_get_object_path(dbusProxy)) : QString();
    bool isBlockChanged = objPath.startsWith(UDISKS_BLOCK_PATH_PREFIX);
    bool isDriveChanged = objPath.startsWith(UDISKS_DRIVE_PATH_PREFIX);
    if (!isBlockChanged && !isDriveChanged) return;

    QMap<Property, QVariant> changes;
    QVariant val = Utils::castFromGVariant(property);
    if (val.type() == QVariant::Map) {
        QVariantMap vmap = val.toMap();
        auto iter = vmap.cbegin();
        while (iter != vmap.cend()) {
            auto key = iter.key();
            auto val = iter.value();
            iter++;
            Property type = Utils::getPropertyByName(key);
            if (type == Property::NotInit) {
                qDebug() << "\tproperty: " << key << "has no mapped type, but value is" << val;
                continue;
            }
            changes.insert(type, val);
        }
    } else {
        qWarning() << "property is not dict" << val;
        return;
    }

    if (changes.isEmpty()) {
        qDebug() << "\tno import changes to report";
        return;
    }

    if (changes.contains(Property::FileSystemMountPoint)) {
        auto mpts = changes.value(Property::FileSystemMountPoint).toStringList();
        Q_EMIT mpts.isEmpty() ? q->mountRemoved(objPath) : q->mountAdded(objPath, mpts.first());
    }
    if (changes.contains(Property::BlockIDLabel)) {
        ; // TODO emit idLabel changed
    }

    if (isBlockChanged)
        Q_EMIT q->propertyChanged(objPath, changes);
    else {
        QSet<QString> blks = blksOfDrive.value(objPath);
        for (const auto &blk: blks)
            Q_EMIT q->propertyChanged(blk, changes);
    }
}

void DFMBlockMonitorPrivate::initDevices()
{
    auto lst = getDevices();
    for (const auto &blk: lst) {
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

DFMBlockMonitor::DFMBlockMonitor(QObject *parent)
    : DFMMonitor (new DFMBlockMonitorPrivate(this), parent)
{
    auto dp = Utils::castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    registerStartMonitor(std::bind(&DFMBlockMonitorPrivate::startMonitor, dp));
    registerStopMonitor(std::bind(&DFMBlockMonitorPrivate::stopMonitor, dp));
    registerMonitorObjectType(std::bind(&DFMBlockMonitorPrivate::monitorObjectType, dp));
    registerGetDevices(std::bind(&DFMBlockMonitorPrivate::getDevices, dp));
    registerCreateDeviceById(std::bind(&DFMBlockMonitorPrivate::createDeviceById, dp, std::placeholders::_1));
}

DFMBlockMonitor::~DFMBlockMonitor()
{
    auto dp = Utils::castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
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
    auto dp = Utils::castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDevice(devspec, opts) : QStringList();
}

QStringList DFMBlockMonitor::resolveDeviceNode(const QString &node, const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDeviceNode(node, opts) : QStringList();
}

/*!
 * \brief DFMBlockMonitor::resolveDeviceFromDrive
 * \param drvObjPath:   object path for drive
 * \return              the associated block object paths
 */
QStringList DFMBlockMonitor::resolveDeviceFromDrive(const QString &drvObjPath)
{
    auto dp = Utils::castClassFromTo<DFMMonitorPrivate, DFMBlockMonitorPrivate>(d.data());
    return dp ? dp->resolveDeviceOfDrive(drvObjPath) : QStringList();
}

