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

#include "dfmprotocolmonitor.h"
#include "private/dfmprotocolmonitor_p.h"
#include "dfmprotocoldevice.h"
#include "base/dfmmountutils.h"

#include <QDebug>
#include <QThread>
#include <QApplication>

extern "C" {
#include <gio/gio.h>
}

DFM_MOUNT_USE_NS

DFMProtocolMonitor::DFMProtocolMonitor(QObject *parent)
    : DFMMonitor (new DFMProtocolMonitorPrivate(this), parent)
{
    auto dp = Utils::castClassFromTo<DFMMonitorPrivate, DFMProtocolMonitorPrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    registerStartMonitor(std::bind(&DFMProtocolMonitorPrivate::startMonitor, dp));
    registerStopMonitor(std::bind(&DFMProtocolMonitorPrivate::stopMonitor, dp));
    registerMonitorObjectType(std::bind(&DFMProtocolMonitorPrivate::monitorObjectType, dp));
    registerGetDevices(std::bind(&DFMProtocolMonitorPrivate::getDevices, dp));
    registerCreateDeviceById(std::bind(&DFMProtocolMonitorPrivate::createDevice, dp, std::placeholders::_1));
}

DFMProtocolMonitor::~DFMProtocolMonitor()
{

}

DFMProtocolMonitorPrivate::DFMProtocolMonitorPrivate(DFMProtocolMonitor *qq)
    : DFMMonitorPrivate (qq)
{
    // gvolumemonitor must be initialized in main thread
    if (QThread::currentThread() != qApp->thread()) {
        qCritical() << "not allow to init protocol monitor in non-main thread";
        abort();
    }
    gVolMonitor = g_volume_monitor_get();
    if (!gVolMonitor) {
        qCritical() << "cannot allocate volume monitor";
        abort();
    }
}

DFMProtocolMonitorPrivate::~DFMProtocolMonitorPrivate()
{
    if (gVolMonitor)
        g_object_unref(gVolMonitor);
    gVolMonitor = nullptr;
}

bool DFMProtocolMonitorPrivate::startMonitor()
{
    if (!gVolMonitor) {
        qCritical() << "monitor is not valid";
        abort();
    }

    ulong handler = 0;
    // ignore drives which will be handled in DFMBlockMonitor
//    handler = g_signal_connect(gVolMonitor, DRIVE_CHANGED, G_CALLBACK(&DFMProtocolMonitorPrivate::onDriveChanged), q);
//    connections.insert(DRIVE_CHANGED, handler);

//    handler = g_signal_connect(gVolMonitor, DRIVE_CONNECTED, G_CALLBACK(&DFMProtocolMonitorPrivate::onDriveConnected), q);
//    connections.insert(DRIVE_CONNECTED, handler);

//    handler = g_signal_connect(gVolMonitor, DRIVE_DISCONNED, G_CALLBACK(&DFMProtocolMonitorPrivate::onDriveDisconnected), q);
//    connections.insert(DRIVE_DISCONNED, handler);

    handler = g_signal_connect(gVolMonitor, MOUNT_ADDED, G_CALLBACK(&DFMProtocolMonitorPrivate::onMountAdded), q);
    connections.insert(MOUNT_ADDED, handler);

    handler = g_signal_connect(gVolMonitor, MOUNT_PRE_UNMOUNT, G_CALLBACK(&DFMProtocolMonitorPrivate::onMountPreUnmount), q);
    connections.insert(MOUNT_PRE_UNMOUNT, handler);

    handler = g_signal_connect(gVolMonitor, MOUNT_REMOVED, G_CALLBACK(&DFMProtocolMonitorPrivate::onMountRemoved), q);
    connections.insert(MOUNT_REMOVED, handler);

    handler = g_signal_connect(gVolMonitor, VOLUME_ADDED, G_CALLBACK(&DFMProtocolMonitorPrivate::onVolumeAdded), q);
    connections.insert(VOLUME_ADDED, handler);

    handler = g_signal_connect(gVolMonitor, VOLUME_CHANGED, G_CALLBACK(&DFMProtocolMonitorPrivate::onVolumeChanged), q);
    connections.insert(VOLUME_CHANGED, handler);

    handler = g_signal_connect(gVolMonitor, VOLUME_REMOVED, G_CALLBACK(&DFMProtocolMonitorPrivate::onVolumeRemoved), q);
    connections.insert(VOLUME_REMOVED, handler);

    qDebug() << "protocol monitor start";
    return true;
}

bool DFMProtocolMonitorPrivate::stopMonitor()
{
    for (auto iter = connections.cbegin(); iter != connections.cend(); iter++)
        g_signal_handler_disconnect(gVolMonitor, iter.value());
    connections.clear();

    qDebug() << "protocol monitor stop";
    return true;
}

DeviceType DFMProtocolMonitorPrivate::monitorObjectType() const
{
    return DeviceType::ProtocolDevice;
}

QStringList DFMProtocolMonitorPrivate::getDevices()
{
    /* all blocks have a GDrive object, but we need to handle CDDA which exactly has a GDrive
     * GDrive   <-->    UDisks::Drive
     * GVolume  <-->    UDisks::Block           which to show on user interface, a 'partiton' entry.
     * GMount   <-->    UDisks::FileSystem      which means a mountable, always have a filesystem.
     * it's not precise though
     *
     * ----GDrive--------GVolume--------GMount-------DeviceType
     *      yes            yes           opt         block devices
     *      no             yes           opt         phones (mpt/ptp/gphoto, etc.)
     *      no             no            opt         virtual devices (smb/ftp/sftp, etc.)
     * -----------------------------------------------------------------------------------
     * opt means the Object is not existed anytime, only if it was mounted, then GMount object generated.
     * so, we need to gather both phones and virtual devices and ignore block devices which will be gathered in DFMBlockMonitor.
     * */

    // second, find those orphan mounts
    GList *mnts = g_volume_monitor_get_mounts(gVolMonitor);
    auto iterMnt = [](gpointer pMnt, gpointer userData) {
        Q_UNUSED(userData);
        GMount *mnt = static_cast<GMount *>(pMnt);
        if (!mnt) return;


        GVolume *vol = g_mount_get_volume(mnt);
        if (vol) { // ignore it
            g_object_unref(vol);
            g_object_unref(mnt);
            return;
        }
        qDebug() << __FUNCTION__ << mnt;

        // TODO
        char *mntName = g_mount_get_name(mnt);
        qDebug() << "\t find orphan mount: " << mntName;
        g_free(mntName);

        g_object_unref(mnt);
    };
    g_list_foreach(mnts, static_cast<GFunc>(iterMnt), nullptr);
    g_list_free(mnts);

    // first, get all volumes which do not have a GDrive.
    GList *vols = g_volume_monitor_get_volumes(gVolMonitor);
    auto iterVol = [](gpointer pVol, gpointer userData) {
        Q_UNUSED(userData);
        GVolume *vol = static_cast<GVolume *>(pVol);
        if (!vol) return;

        GDrive *drv = g_volume_get_drive(vol);
        if (drv) { // ignore blocks
            g_object_unref(drv);
            g_object_unref(vol);
            return;
        }

        // TODO
        GMount *mnt = g_volume_get_mount(vol);
        char *volName = g_volume_get_name(vol);
        bool mountable = g_volume_can_mount(vol);
        qDebug() << "\t find volume: " << volName << ", have mount? " << (mnt != nullptr)
                 << ", can mount?" << mountable;
        g_free(volName);
        if (mnt) g_object_unref(mnt);

        g_object_unref(vol);
    };
    g_list_foreach(vols, static_cast<GFunc>(iterVol), nullptr);
    g_list_free(vols);

    return QStringList();
}

QSharedPointer<DFMDevice> DFMProtocolMonitorPrivate::createDevice(const QString &id)
{
    // TODO
    return nullptr;
}

void DFMProtocolMonitorPrivate::onDriveChanged(GVolumeMonitor *monitor, GDrive *drive, gpointer userData)
{
    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;

    char *drvName = g_drive_get_name(drive);
    qDebug() << "\tdrvName:" << drvName;
    g_free(drvName);

    // TODO
}

void DFMProtocolMonitorPrivate::onDriveConnected(GVolumeMonitor *monitor, GDrive *drive, gpointer userData)
{
    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;

    char *drvName = g_drive_get_name(drive);
    qDebug() << "\tdrvName:" << drvName;
    g_free(drvName);

    // TODO
}

void DFMProtocolMonitorPrivate::onDriveDisconnected(GVolumeMonitor *monitor, GDrive *drive, gpointer userData)
{
    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;

    char *drvName = g_drive_get_name(drive);
    qDebug() << "\tdrvName:" << drvName;
    g_free(drvName);

    // TODO
}

void DFMProtocolMonitorPrivate::onMountAdded(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    if (hasDrive(mount)) // don't handle real block devices
        return;

    qDebug() << __FUNCTION__ << mount;

    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;

    qDebug() << "\tmount is shadowd: " << g_mount_is_shadowed(mount);

    char *mntName = g_mount_get_name(mount);
    qDebug() << "\tmntName:" << mntName;
    g_free(mntName);
    auto *mntRoot = g_mount_get_root(mount);
    if (mntRoot) {
        char *mntPath = g_file_get_path(mntRoot);
        qDebug() << "\tmounted at: " << mntPath;
        g_free(mntPath);
        g_object_unref(mntRoot);
    }

    auto *vol = g_mount_get_volume(mount);
    auto *drv = g_mount_get_drive(mount);
    qDebug() << "\tmount has volume: " << (vol != nullptr);
    qDebug() << "\tmount has drive: " << (drv != nullptr);
    if (vol)
        g_object_unref(vol);
    if (drv)
        g_object_unref(drv);

    // TODO why this slot is triggered twice when a phone mounted via mtp/gphoto?
}

void DFMProtocolMonitorPrivate::onMountChanged(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    if (hasDrive(mount)) // don't handle real block devices
        return;

    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;
    char *mntName = g_mount_get_name(mount);
    qDebug() << "\tmntName:" << mntName;
    g_free(mntName);

    auto *vol = g_mount_get_volume(mount);
    auto *drv = g_mount_get_drive(mount);
    qDebug() << "\tmount has volume: " << (vol != nullptr);
    qDebug() << "\tmount has drive: " << (drv != nullptr);
    if (vol)
        g_object_unref(vol);
    if (drv)
        g_object_unref(drv);

    // TODO
}

void DFMProtocolMonitorPrivate::onMountPreUnmount(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    if (hasDrive(mount)) // don't handle real block devices
        return;

    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;
    char *mntName = g_mount_get_name(mount);
    qDebug() << "\tmntName:" << mntName;
    g_free(mntName);

    auto *vol = g_mount_get_volume(mount);
    auto *drv = g_mount_get_drive(mount);
    qDebug() << "\tmount has volume: " << (vol != nullptr);
    qDebug() << "\tmount has drive: " << (drv != nullptr);
    if (vol)
        g_object_unref(vol);
    if (drv)
        g_object_unref(drv);

    // TODO
}

void DFMProtocolMonitorPrivate::onMountRemoved(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    if (hasDrive(mount)) // don't handle real block devices
        return;

    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;
    char *mntName = g_mount_get_name(mount);
    qDebug() << "\tmntName:" << mntName;
    g_free(mntName);

    auto *vol = g_mount_get_volume(mount);
    auto *drv = g_mount_get_drive(mount);
    qDebug() << "\tmount has volume: " << (vol != nullptr);
    qDebug() << "\tmount has drive: " << (drv != nullptr);
    if (vol)
        g_object_unref(vol);
    if (drv)
        g_object_unref(drv);

    // TODO
}

void DFMProtocolMonitorPrivate::onVolumeAdded(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    if (hasDrive(volume)) // don't handle real block devices
        return;

    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;

    char *volName = g_volume_get_name(volume);
    qDebug() << "\tvolName:" << volName;
    g_free(volName);

    // TODO
}

void DFMProtocolMonitorPrivate::onVolumeChanged(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    if (hasDrive(volume)) // don't handle real block devices
        return;

    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;
    char *volName = g_volume_get_name(volume);
    qDebug() << "\tvolName:" << volName;
    g_free(volName);

    // TODO
}

void DFMProtocolMonitorPrivate::onVolumeRemoved(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    if (hasDrive(volume)) // don't handle real block devices
        return;

    auto *q = static_cast<DFMProtocolMonitor *>(userData);
    Q_ASSERT(q);
    qDebug() << __FUNCTION__;
    char *volName = g_volume_get_name(volume);
    qDebug() << "\tvolName:" << volName;
    g_free(volName);

    // TODO
}

bool DFMProtocolMonitorPrivate::hasDrive(GMount *mount)
{
    if (!mount)
        return false;

    auto *drv = g_mount_get_drive(mount);
    if (drv) {
        g_object_unref(drv);
        return true;
    }
    return false;
}

bool DFMProtocolMonitorPrivate::hasDrive(GVolume *volume)
{
    if (!volume)
        return false;

    auto *drv = g_volume_get_drive(volume);
    if (drv) {
        g_object_unref(drv);
        return true;
    }
    return false;
}

