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
#include "private/dfmprotocoldevice_p.h"
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
    auto protocolDev = new DFMProtocolDevice(id, dev.volume, dev.mount, gVolMonitor, q);
    QSharedPointer<DFMDevice> ret;
    ret.reset(protocolDev);
    pdevices.append(protocolDev);
    q->connect(protocolDev, &DFMProtocolDevice::destroyed, q, [this, protocolDev](){
        pdevices.removeAll(protocolDev);
    });
    return ret;
}

QSharedPointer<DFMDevice> DFMProtocolMonitorPrivate::createDeviceByMount()
{
// TODO
    return nullptr;
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
        g_autoptr(GVolume) vol = g_mount_get_volume(mnt);
        if (vol) { // ignore it
            g_object_unref(mnt);
            return;
        }

        // TODO: cannot bind an iphone AFC mount with its volume, it does not emit a mountAdded signal when monitor start.
        // nemo/nautilus... show volume and mount seperated. seems like an upstream issue or designed so.

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

        g_autoptr(GDrive) drv = g_volume_get_drive(vol);
        if (drv) { // ignore blocks
            g_object_unref(vol);
            return;
        }

        g_autoptr(GFile) willMountAt = g_volume_get_activation_root(vol);
        if (willMountAt) {
            g_autofree char *cpath = g_file_get_path(willMountAt);
            QString mpt(cpath);

            auto existMountKey = d->findAssociatedMount(mpt);
            if (!existMountKey.isEmpty()) {
                d->devices[existMountKey].volume = vol;
            } else {
                DeviceCache dev;
                dev.uuid = QUuid::createUuid().toString();
                dev.volume = vol;
                d->devices.insert(dev.uuid, dev);
            }
        } else {
            DeviceCache dev;
            dev.uuid = QUuid::createUuid().toString();
            dev.volume = vol;
            d->devices.insert(dev.uuid, dev);
        }

        g_object_unref(vol);
    };
    g_list_foreach(vols, static_cast<GFunc>(iterVol), this);
    g_list_free(vols);

//    printDevices();
}

void DFMProtocolMonitorPrivate::onMountAdded(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(mount)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    g_autofree char *mntName = g_mount_get_name(mount);
    qDebug() << __FUNCTION__ << mntName;

    QString mpt = DFMProtocolDevicePrivate::mountPoint(mount);
    auto id = d->findAssociatedMount(mpt);
    if (!id.isEmpty()) {
        g_autoptr(GVolume) vol = g_mount_get_volume(mount);
        // remove same orphan volume
        g_autofree char *volId = g_volume_get_identifier(vol, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
        auto key = d->findOrphanVolume(volId);
        if (!key.isEmpty())
            d->devices.remove(key);
        d->devices[id].volume = vol;

        for (auto dev: d->pdevices) {
            if (dev->path() == id) {
                dev->setVolume(vol);
                qDebug() << "dev's volume settled";
            }
        }
//        d->printDevices();
    } else {
        DeviceCache dev;
        dev.uuid = QUuid::createUuid().toString();
        dev.mount = mount;
        d->devices.insert(dev.uuid, dev);
//        d->printDevices();

        Q_EMIT d->q->mountAdded(dev.uuid, mpt);
    }
}

void DFMProtocolMonitorPrivate::onMountChanged(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(mount)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;
}

void DFMProtocolMonitorPrivate::onMountRemoved(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(mount)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    g_autofree char *mntName = g_mount_get_name(mount);
    qDebug() << __FUNCTION__ << mntName;

    QString mpt = DFMProtocolDevicePrivate::mountPoint(mount);
    auto id = d->findAssociatedMount(mpt);
    if (!id.isEmpty()) {
        if (d->devices[id].volume)
            d->devices[id].mount = nullptr;
        else
            d->devices.remove(id);

        Q_EMIT d->q->mountRemoved(id);
    }

//    d->printDevices();
}

void DFMProtocolMonitorPrivate::onVolumeAdded(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    g_autofree char *volName = g_volume_get_name(volume);
    qDebug() << __FUNCTION__ << volName;

    DeviceCache dev;
    dev.uuid = QUuid::createUuid().toString();
    dev.volume = volume;
    d->devices.insert(dev.uuid, dev);

    Q_EMIT d->q->deviceAdded(dev.uuid);

//    d->printDevices();
}

void DFMProtocolMonitorPrivate::onVolumeChanged(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;
}

void DFMProtocolMonitorPrivate::onVolumeRemoved(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume)) // don't handle real block devices
        return;
    auto d = static_cast<DFMProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    g_autofree char *volName = g_volume_get_name(volume);
    qDebug() << __FUNCTION__ << volName;

    g_autofree char *unixDev = g_volume_get_identifier(volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    const auto &&removedItems = d->removeVolumes(unixDev);

    for (const auto &item: removedItems)
        Q_EMIT d->q->deviceRemoved(item);

//    d->printDevices();
}

bool DFMProtocolMonitorPrivate::hasDrive(GMount *mount)
{
    if (!mount)
        return false;
    g_autoptr(GDrive) drv = g_mount_get_drive(mount);
    return drv ? true : false;
}

bool DFMProtocolMonitorPrivate::hasDrive(GVolume *volume)
{
    if (!volume)
        return false;
    g_autoptr(GDrive) drv = g_volume_get_drive(volume);
    return drv ? true : false;
}

QString DFMProtocolMonitorPrivate::findAssociatedMount(const QString &mpt)
{
    auto iter = devices.cbegin();
    while (iter != devices.cend()) {
        const auto &dev = iter.value();
        auto mnt = dev.mount;
        if (mnt) {
            auto mntPath = DFMProtocolDevicePrivate::mountPoint(mnt);
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
            g_autofree char *id = g_volume_get_identifier(dev.volume, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
            if (QString(id) == volId)
                return iter.key();
        }
        iter += 1;
    }
    return "";
}

QStringList DFMProtocolMonitorPrivate::removeVolumes(const QString &volId)
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
    return waitToRemove;
}

void DFMProtocolMonitorPrivate::printDevices()
{
    auto iter = devices.cbegin();
    while (iter != devices.cend()) {
        const auto &dev = iter.value();
        auto mnt = dev.mount;
        auto vol = dev.volume;
        QString volName, mntName, mntPath;
        if (vol) {
            g_autofree char *cname = g_volume_get_name(vol);
            volName = QString(cname);
        }
        if (mnt) {
            g_autofree char *cname = g_mount_get_name(mnt);
            mntName = QString(cname);
            mntPath = DFMProtocolDevicePrivate::mountPoint(mnt);
        }

        qDebug() << "\t" << iter.key() << ", volName: " << volName << ", mntName: " << mntName
                 << ", mount at: " << mntPath;

        iter += 1;
    }
    qDebug() << "\n\n";
}

