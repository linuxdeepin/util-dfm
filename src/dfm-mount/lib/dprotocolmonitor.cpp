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

#include "base/dmountutils.h"
#include "dprotocolmonitor.h"
#include "dprotocoldevice.h"
#include "private/dprotocolmonitor_p.h"
#include "private/dprotocoldevice_p.h"

#include <QUuid>
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <QRegularExpression>

extern "C" {
#include <gio/gio.h>
#include <gio/gunixmounts.h>
}

DFM_MOUNT_USE_NS

DProtocolMonitor::DProtocolMonitor(QObject *parent)
    : DDeviceMonitor(new DProtocolMonitorPrivate(this), parent)
{
    auto dp = Utils::castClassFromTo<DDeviceMonitorPrivate, DProtocolMonitorPrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    registerStartMonitor(std::bind(&DProtocolMonitorPrivate::startMonitor, dp));
    registerStopMonitor(std::bind(&DProtocolMonitorPrivate::stopMonitor, dp));
    registerMonitorObjectType(std::bind(&DProtocolMonitorPrivate::monitorObjectType, dp));
    registerGetDevices(std::bind(&DProtocolMonitorPrivate::getDevices, dp));
    registerCreateDeviceById(std::bind(&DProtocolMonitorPrivate::createDevice, dp, std::placeholders::_1));
}

DProtocolMonitor::~DProtocolMonitor()
{
}

DProtocolMonitorPrivate::DProtocolMonitorPrivate(DProtocolMonitor *qq)
    : DDeviceMonitorPrivate(qq)
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

DProtocolMonitorPrivate::~DProtocolMonitorPrivate()
{
    if (gVolMonitor)
        g_object_unref(gVolMonitor);
    gVolMonitor = nullptr;
}

bool DProtocolMonitorPrivate::startMonitor()
{
    if (!gVolMonitor) {
        qCritical() << "monitor is not valid";
        abort();
    }

    ulong handler = 0;

    handler = g_signal_connect(gVolMonitor, MOUNT_ADDED, G_CALLBACK(&DProtocolMonitorPrivate::onMountAdded), this);
    connections.insert(MOUNT_ADDED, handler);

    //    handler = g_signal_connect(gVolMonitor, MOUNT_CHANGED, G_CALLBACK(&DProtocolMonitorPrivate::onMountChanged), this);
    //    connections.insert(MOUNT_CHANGED, handler);

    handler = g_signal_connect(gVolMonitor, MOUNT_REMOVED, G_CALLBACK(&DProtocolMonitorPrivate::onMountRemoved), this);
    connections.insert(MOUNT_REMOVED, handler);

    handler = g_signal_connect(gVolMonitor, VOLUME_ADDED, G_CALLBACK(&DProtocolMonitorPrivate::onVolumeAdded), this);
    connections.insert(VOLUME_ADDED, handler);

    //    handler = g_signal_connect(gVolMonitor, VOLUME_CHANGED, G_CALLBACK(&DProtocolMonitorPrivate::onVolumeChanged), this);
    //    connections.insert(VOLUME_CHANGED, handler);

    handler = g_signal_connect(gVolMonitor, VOLUME_REMOVED, G_CALLBACK(&DProtocolMonitorPrivate::onVolumeRemoved), this);
    connections.insert(VOLUME_REMOVED, handler);

    qDebug() << "protocol monitor start";
    return true;
}

bool DProtocolMonitorPrivate::stopMonitor()
{
    for (auto iter = connections.cbegin(); iter != connections.cend(); iter++)
        g_signal_handler_disconnect(gVolMonitor, iter.value());
    connections.clear();

    qDebug() << "protocol monitor stop";
    return true;
}

DeviceType DProtocolMonitorPrivate::monitorObjectType() const
{
    return DeviceType::kProtocolDevice;
}

QStringList DProtocolMonitorPrivate::getDevices()
{
    return cachedDevices.toList();
}

QSharedPointer<DDevice> DProtocolMonitorPrivate::createDevice(const QString &id)
{
    auto dev = new DProtocolDevice(id, gVolMonitor, nullptr);

    // for updating the mounthandler of device, if the device exists.
    QObject::connect(q, &DProtocolMonitor::mountAdded, dev, &DProtocolDevice::mounted);
    QObject::connect(q, &DProtocolMonitor::mountRemoved, dev, &DProtocolDevice::unmounted);

    QSharedPointer<DDevice> ret;
    ret.reset(dev);
    return ret;
}

QSharedPointer<DDevice> DProtocolMonitorPrivate::createDeviceByMount()
{
    // TODO
    return nullptr;
}

void DProtocolMonitorPrivate::initDeviceList()
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
     * so, we need to gather both phones and virtual devices and ignore block devices which will be gathered in DBlockMonitor.
     * */

    // first, get all volumes which do not have a GDrive.
    GList *vols = g_volume_monitor_get_volumes(gVolMonitor);
    auto iterVol = [](gpointer pVol, gpointer userData) {
        auto d = static_cast<DProtocolMonitorPrivate *>(userData);
        Q_ASSERT(d);

        GVolume *vol = static_cast<GVolume *>(pVol);
        if (!vol) return;

        g_autoptr(GDrive) drv = g_volume_get_drive(vol);
        if (drv)   // ignore blocks
            return;

        g_autoptr(GFile) activationRoot = g_volume_get_activation_root(vol);
        if (activationRoot) {
            g_autofree char *curi = g_file_get_uri(activationRoot);
            d->cachedDevices.insert(curi);
        } else {
            g_autofree char *volId = g_volume_get_identifier(vol, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
            qWarning() << "protocol: cannot get the root of " << volId;
        }
    };
    g_list_foreach(vols, static_cast<GFunc>(iterVol), this);
    g_list_free_full(vols, g_object_unref);

    // second, find those orphan mounts
    GList *mnts = g_volume_monitor_get_mounts(gVolMonitor);
    auto iterMnt = [](gpointer pMnt, gpointer userData) {
        auto d = static_cast<DProtocolMonitorPrivate *>(userData);
        Q_ASSERT(d);

        GMount *mnt = static_cast<GMount *>(pMnt);
        if (!mnt) return;
        g_autoptr(GVolume) vol = g_mount_get_volume(mnt);
        if (vol)   // only find orphan mounts
            return;

        // TODO: cannot bind an iphone AFC mount with its volume, it does not emit a mountAdded signal when monitor start.
        // nemo/nautilus... show volume and mount seperated. seems like an upstream issue or designed so.

        g_autoptr(GFile) root = g_mount_get_root(mnt);
        if (root) {
            g_autofree char *curi = g_file_get_uri(root);
            auto mpt = DProtocolDevicePrivate::mountPoint(mnt);
            if (!isNativeMount(mpt) && !isMountByOther(mpt))
                d->cachedDevices.insert(curi);
        } else {
            g_autofree char *cname = g_mount_get_name(mnt);
            qWarning() << "protocol: cannot get the root of " << cname;
        }
    };
    g_list_foreach(mnts, static_cast<GFunc>(iterMnt), this);
    g_list_free_full(mnts, g_object_unref);
}

void DProtocolMonitorPrivate::onMountAdded(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    auto mpt = DProtocolDevicePrivate::mountPoint(mount);
    if (isNativeMount(mpt) || hasDrive(mount) || isMountByOther(mpt))
        return;

    auto d = static_cast<DProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    g_autoptr(GFile) mntRoot = g_mount_get_root(mount);
    if (mntRoot) {
        g_autofree char *curi = g_file_get_uri(mntRoot);
        d->cachedDevices.insert(curi);
        Q_EMIT d->q->mountAdded(curi, mpt);
    } else {
        qWarning() << "protocol: cannot get the root of " << mpt;
    }
}

void DProtocolMonitorPrivate::onMountChanged(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(mount))   // don't handle real block devices
        return;
    auto d = static_cast<DProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;
}

void DProtocolMonitorPrivate::onMountRemoved(GVolumeMonitor *monitor, GMount *mount, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(mount))   // don't handle real block devices
        return;
    auto d = static_cast<DProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    auto mpt = DProtocolDevicePrivate::mountPoint(mount);
    //    if (!mpt.isEmpty() && isNativeMount(mpt))   // don't report block device unmounted.
    //        return;

    g_autoptr(GFile) mntRoot = g_mount_get_root(mount);
    if (mntRoot) {
        g_autofree char *curi = g_file_get_uri(mntRoot);
        if (isOrphanMount(mount))
            d->cachedDevices.remove(curi);
        Q_EMIT d->q->mountRemoved(curi);
    } else {
        qWarning() << "protocol: cannot get the root of " << mpt;
    }
}

void DProtocolMonitorPrivate::onVolumeAdded(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume))   // don't handle real block devices
        return;
    auto d = static_cast<DProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    g_autoptr(GFile) activationRoot = g_volume_get_activation_root(volume);
    if (activationRoot) {
        g_autofree char *curi = g_file_get_uri(activationRoot);
        d->cachedDevices.insert(curi);
        Q_EMIT d->q->deviceAdded(curi);
    } else {
        qWarning() << "protocol: cannot get the root of " << volume;
    }
}

void DProtocolMonitorPrivate::onVolumeChanged(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume))   // don't handle real block devices
        return;
    auto d = static_cast<DProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    qDebug() << __FUNCTION__;
}

void DProtocolMonitorPrivate::onVolumeRemoved(GVolumeMonitor *monitor, GVolume *volume, gpointer userData)
{
    Q_UNUSED(monitor);
    if (hasDrive(volume))   // don't handle real block devices
        return;
    auto d = static_cast<DProtocolMonitorPrivate *>(userData);
    Q_ASSERT(d);

    g_autoptr(GFile) activationRoot = g_volume_get_activation_root(volume);
    if (activationRoot) {
        g_autofree char *curi = g_file_get_uri(activationRoot);
        d->cachedDevices.remove(curi);
        Q_EMIT d->q->deviceRemoved(curi);
    } else {
        qWarning() << "protocol: cannot get the root of " << volume;
    }
}

bool DProtocolMonitorPrivate::hasDrive(GMount *mount)
{
    if (!mount)
        return false;
    g_autoptr(GDrive) drv = g_mount_get_drive(mount);
    return drv ? true : false;
}

bool DProtocolMonitorPrivate::hasDrive(GVolume *volume)
{
    if (!volume)
        return false;
    g_autoptr(GDrive) drv = g_volume_get_drive(volume);
    return drv ? true : false;
}

bool DProtocolMonitorPrivate::isNativeMount(const QString &mpt)
{
    if (mpt.isEmpty())
        return false;

    std::string s = mpt.toStdString();
    GUnixMountEntry_autoptr entry = g_unix_mount_for(s.data(), nullptr);
    if (entry) {
        QString devPath = g_unix_mount_get_device_path(entry);
        if (devPath.startsWith("/dev/"))
            return true;
    }
    return false;
}

bool DProtocolMonitorPrivate::isMountByOther(const QString &mpt)
{
    QRegularExpression re("^/media/(.*)/smbmounts");
    auto match = re.match(mpt);
    if (!match.hasMatch())   // mount which not mouted at preseted path regards as a normal mount
        return false;

    auto user = match.captured(1);
    return user != Utils::currentUser();
}

bool DProtocolMonitorPrivate::isOrphanMount(GMount *mount)
{
    g_autoptr(GFile) mntRoot = g_mount_get_root(mount);
    g_autofree char *curi = g_file_get_uri(mntRoot);
    QString uri(curi);
    if (uri.startsWith("smb") || uri.startsWith("ftp") || uri.startsWith("sftp"))
        return true;

    bool isOrphan = true;
    g_autoptr(GVolumeMonitor) monitor = g_volume_monitor_get();
    GList *vols = g_volume_monitor_get_volumes(monitor);
    while (vols) {
        auto *vol = reinterpret_cast<GVolume *>(vols->data);
        g_autoptr(GFile) volRoot = g_volume_get_activation_root(vol);
        if (volRoot) {
            g_autofree char *volUri = g_file_get_uri(volRoot);
            if (g_strcmp0(curi, volUri) == 0) {
                isOrphan = false;
                break;
            }
        }
        vols = vols->next;
    }
    g_list_free_full(vols, g_object_unref);

    return isOrphan;
}
