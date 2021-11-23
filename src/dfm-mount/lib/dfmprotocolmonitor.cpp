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

#include <QUuid>
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

    initDeviceList();
}

DFMProtocolMonitorPrivate::~DFMProtocolMonitorPrivate()
{
    if (gVolMonitor)
        g_object_unref(gVolMonitor);
    gVolMonitor = nullptr;

    devices.clear();
}

bool DFMProtocolMonitorPrivate::startMonitor()
{
    if (!gVolMonitor) {
        qCritical() << "monitor is not valid";
        abort();
    }

    ulong handler = 0;
#if 0// ignore drives which will be handled in DFMBlockMonitor
    handler = g_signal_connect(gVolMonitor, DRIVE_CHANGED, G_CALLBACK(&DFMProtocolMonitorPrivate::onDriveChanged), this);
    connections.insert(DRIVE_CHANGED, handler);

    handler = g_signal_connect(gVolMonitor, DRIVE_CONNECTED, G_CALLBACK(&DFMProtocolMonitorPrivate::onDriveConnected), this);
    connections.insert(DRIVE_CONNECTED, handler);

    handler = g_signal_connect(gVolMonitor, DRIVE_DISCONNED, G_CALLBACK(&DFMProtocolMonitorPrivate::onDriveDisconnected), this);
    connections.insert(DRIVE_DISCONNED, handler);

    handler = g_signal_connect(gVolMonitor, MOUNT_PRE_UNMOUNT, G_CALLBACK(&DFMProtocolMonitorPrivate::onMountPreUnmount), this);
    connections.insert(MOUNT_PRE_UNMOUNT, handler);
#endif

    handler = g_signal_connect(gVolMonitor, MOUNT_ADDED, G_CALLBACK(&DFMProtocolMonitorPrivate::onMountAdded), this);
    connections.insert(MOUNT_ADDED, handler);

//    handler = g_signal_connect(gVolMonitor, MOUNT_CHANGED, G_CALLBACK(&DFMProtocolMonitorPrivate::onMountChanged), this);
//    connections.insert(MOUNT_CHANGED, handler);

    handler = g_signal_connect(gVolMonitor, MOUNT_REMOVED, G_CALLBACK(&DFMProtocolMonitorPrivate::onMountRemoved), this);
    connections.insert(MOUNT_REMOVED, handler);

    handler = g_signal_connect(gVolMonitor, VOLUME_ADDED, G_CALLBACK(&DFMProtocolMonitorPrivate::onVolumeAdded), this);
    connections.insert(VOLUME_ADDED, handler);

//    handler = g_signal_connect(gVolMonitor, VOLUME_CHANGED, G_CALLBACK(&DFMProtocolMonitorPrivate::onVolumeChanged), this);
//    connections.insert(VOLUME_CHANGED, handler);

    handler = g_signal_connect(gVolMonitor, VOLUME_REMOVED, G_CALLBACK(&DFMProtocolMonitorPrivate::onVolumeRemoved), this);
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
    return devices.keys();
}

QSharedPointer<DFMDevice> DFMProtocolMonitorPrivate::createDevice(const QString &id)
{
    if (!devices.contains(id))
        return nullptr;
    const auto &dev = devices.value(id);
    return QSharedPointer<DFMProtocolDevice>(new DFMProtocolDevice(id, dev.volume, dev.mount, q));
}

void DFMProtocolMonitorPrivate::initDeviceList()
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

    /* and I also found that, for phone mounts, the devices I got from g_volume_monitor_get_mounts cannot associate with a volume,
     * they are all orphan, but when the monitor is start, a new mount which has the same `mountpoint` is added and has a volume.
     * I think this is a kind of mechanism helps us to link the orphan mount to a specific volume.
     * */

    // second, find those orphan mounts
    GList *mnts = g_volume_monitor_get_mounts(gVolMonitor);
    auto iterMnt = [](gpointer pMnt, gpointer userData) {
        auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
        Q_ASSERT(d);

        GMount *mnt = static_cast<GMount *>(pMnt);
        if (!mnt) return;
        GVolume *vol = g_mount_get_volume(mnt);
        if (vol) { // ignore it
            g_object_unref(vol);
            g_object_unref(mnt);
            return;
        }

        char *mntName = g_mount_get_name(mnt);
        qDebug() << "\t find orphan mount: " << mntName;
        g_free(mntName);

        DeviceCache dev;
        dev.uuid = QUuid::createUuid().toString();
        dev.mount = mnt;
        d->devices.insert(dev.uuid, dev);

        g_object_unref(mnt);
    };
    g_list_foreach(mnts, static_cast<GFunc>(iterMnt), this);
    g_list_free(mnts);

    // first, get all volumes which do not have a GDrive.
    GList *vols = g_volume_monitor_get_volumes(gVolMonitor);
    auto iterVol = [](gpointer pVol, gpointer userData) {
        auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
        Q_ASSERT(d);

        GVolume *vol = static_cast<GVolume *>(pVol);
        if (!vol) return;

        GDrive *drv = g_volume_get_drive(vol);
        if (drv) { // ignore blocks
            g_object_unref(drv);
            g_object_unref(vol);
            return;
        }

        GMount *mnt = g_volume_get_mount(vol);
        char *volName = g_volume_get_name(vol);
        bool mountable = g_volume_can_mount(vol);
        qDebug() << "\t find volume: " << volName << ", have mount? " << (mnt != nullptr)
                 << ", can mount?" << mountable;
        g_free(volName);
        if (mnt) g_object_unref(mnt);

        DeviceCache dev;
        dev.uuid = QUuid::createUuid().toString();
        dev.volume = vol;
        d->devices.insert(dev.uuid, dev);

        g_object_unref(vol);
    };
    g_list_foreach(vols, static_cast<GFunc>(iterVol), this);
    g_list_free(vols);

    qDebug() << devices;
}

#if 0
void DFMProtocolMonitorPrivate::onDriveChanged(GVolumeMonitor *monitor, GDrive *drive, gpointer userData)
{
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);
    qDebug() << __FUNCTION__;

    char *drvName = g_drive_get_name(drive);
    qDebug() << "\tdrvName:" << drvName;
    g_free(drvName);

    // TODO
}

void DFMProtocolMonitorPrivate::onDriveConnected(GVolumeMonitor *monitor, GDrive *drive, gpointer userData)
{
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);
    qDebug() << __FUNCTION__;

    char *drvName = g_drive_get_name(drive);
    qDebug() << "\tdrvName:" << drvName;
    g_free(drvName);

    // TODO
}

void DFMProtocolMonitorPrivate::onDriveDisconnected(GVolumeMonitor *monitor, GDrive *drive, gpointer userData)
{
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);
    qDebug() << __FUNCTION__;

    char *drvName = g_drive_get_name(drive);
    qDebug() << "\tdrvName:" << drvName;
    g_free(drvName);

    // TODO
}
#endif

void DFMProtocolMonitorPrivate::onMountAdded(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);

    if (hasDrive(mount)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;

    QString mpt = getMountPoint(mount);
    auto id = d->findDirectMount(mpt);
    if (!id.isEmpty()) {
        auto vol = g_mount_get_volume(mount);
        // remove same orphan volume
        char *volId = g_volume_get_identifier(vol, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
        auto key = d->findOrphanVolume(volId);
        g_free(volId);
        if (!key.isEmpty())
            d->devices.remove(key);
        d->devices[id].volume = vol;
    } else {
        DeviceCache dev;
        dev.uuid = QUuid::createUuid().toString();
        dev.mount = mount;
        d->devices.insert(dev.uuid, dev);
    }

    qDebug() << "\t    " << d->devices;
}

void DFMProtocolMonitorPrivate::onMountChanged(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(mount)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;

    char *mntName = g_mount_get_name(mount);
    g_free(mntName);

    auto vol = g_mount_get_volume(mount);
    auto drv = g_mount_get_drive(mount);
    if (vol)
        g_object_unref(vol);
    if (drv)
        g_object_unref(drv);
}
#if 0
void DFMProtocolMonitorPrivate::onMountPreUnmount(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    if (hasDrive(mount)) // don't handle real block devices
        return;

    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);
    qDebug() << __FUNCTION__;
    char *mntName = g_mount_get_name(mount);
    qDebug() << "\tmntName:" << mntName;
    g_free(mntName);

    auto vol = g_mount_get_volume(mount);
    auto drv = g_mount_get_drive(mount);
    qDebug() << "\tmount has volume: " << (vol != nullptr);
    qDebug() << "\tmount has drive: " << (drv != nullptr);
    if (vol)
        g_object_unref(vol);
    if (drv)
        g_object_unref(drv);

    // TODO
}
#endif
void DFMProtocolMonitorPrivate::onMountRemoved(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(mount)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;

    char *mntName = g_mount_get_name(mount);
    qDebug() << "\tmntName:" << mntName;
    g_free(mntName);

    QString mpt = getMountPoint(mount);
    auto id = d->findDirectMount(mpt);
    if (!id.isEmpty()) {
        if (d->devices[id].volume)
            d->devices[id].mount = nullptr;
        else
            d->devices.remove(id);
    }

    qDebug() << "\t    " << d->devices;
}

void DFMProtocolMonitorPrivate::onVolumeAdded(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;

    DeviceCache dev;
    dev.uuid = QUuid::createUuid().toString();
    dev.volume = volume;
    d->devices.insert(dev.uuid, dev);

    qDebug() << "\t    " << d->devices;
}

void DFMProtocolMonitorPrivate::onVolumeChanged(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume)) // don't handle real block devices
        return;

    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);
    qDebug() << __FUNCTION__;
    char *volName = g_volume_get_name(volume);
    qDebug() << "\tvolName:" << volName << g_volume_get_identifier(volume, "unix-device");
    g_free(volName);

    // TODO
}

void DFMProtocolMonitorPrivate::onVolumeRemoved(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;

    char *unixDev = g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    d->removeVolumes(unixDev);

    qDebug() << "\t    " << d->devices;
}

bool DFMProtocolMonitorPrivate::hasDrive(GMount *mount)
{
    if (!mount)
        return false;

    auto drv = g_mount_get_drive(mount);
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

    auto drv = g_volume_get_drive(volume);
    if (drv) {
        g_object_unref(drv);
        return true;
    }
    return false;
}

QString DFMProtocolMonitorPrivate::getMountPoint(GMount *mount)
{
    QString mpt;
    auto mntRoot = g_mount_get_root(mount);
    if (mntRoot) {
        char *mntPath = g_file_get_path(mntRoot);
        mpt = QString(mntPath);
        g_free(mntPath);
        g_object_unref(mntRoot);
    }
    return mpt;
}

QString DFMProtocolMonitorPrivate::findDirectMount(const QString &mpt)
{
    auto iter = devices.cbegin();
    while (iter != devices.cend()) {
        const auto &dev = iter.value();
        auto mnt = dev.mount;
        if (mnt) {
            auto mntPath = getMountPoint(mnt);
            if (mntPath == mpt)
                return iter.key();
        }
        iter += 1;
    }
    return "";
}

QString DFMProtocolMonitorPrivate::findOrphanVolume(const QString &volId)
{
    auto iter = devices.cbegin();
    while (iter != devices.cend()) {
        const auto &dev = iter.value();
        if (dev.mount == nullptr && dev.volume) {
            char *id = g_volume_get_identifier(dev.volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
            if (QString(id) == volId) {
                g_free(id);
                return iter.key();
            }
            g_free(id);
        }

        iter += 1;
    }
    return "";
}

void DFMProtocolMonitorPrivate::removeVolumes(const QString &volId)
{
    QStringList waitToRemove;
    auto iter = devices.cbegin();
    while (iter != devices.cend()) {
        const auto &dev = iter.value();
        if (dev.volume) {
            char *id = g_volume_get_identifier(dev.volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
            if (volId == QString(id))
                waitToRemove << dev.uuid;
            g_free(id);
        }
        iter += 1;
    }
    for (const auto &key: waitToRemove)
        devices.remove(key);
}

