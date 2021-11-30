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
#ifndef DFMPROTOCOLMONITOR_P_H
#define DFMPROTOCOLMONITOR_P_H

#include "dfmprotocolmonitor.h"
#include "private/dfmmonitor_p.h"

#include <QMap>
#include <QDebug>

typedef struct _GVolumeMonitor GVolumeMonitor;
typedef struct _GDrive GDrive;
typedef struct _GMount GMount;
typedef struct _GVolume GVolume;
typedef void *gpointer;

DFM_MOUNT_BEGIN_NS

#define DRIVE_CHANGED "drive-changed"
#define DRIVE_CONNECTED "drive-connected"
#define DRIVE_DISCONNED "drive-disconnected"
#define MOUNT_ADDED "mount-added"
#define MOUNT_CHANGED "mount-changed"
#define MOUNT_PRE_UNMOUNT "mount-pre-unmount"
#define MOUNT_REMOVED "mount-removed"
#define VOLUME_ADDED "volume-added"
#define VOLUME_CHANGED "volume-changed"
#define VOLUME_REMOVED "volume-removed"

struct DeviceCache
{
    QString uuid {};
    GMount *mount { nullptr };
    GVolume *volume { nullptr };

    friend QDebug operator<<(QDebug debug, const DeviceCache &device)
    {
        bool hasMount, hasVolume;
        hasMount = (device.mount != nullptr);
        hasVolume = (device.volume != nullptr);
        debug << QString("{ mpt: %1, mount: %2, volume: %3 }").arg(device.uuid).arg(hasMount).arg(hasVolume);
        return debug;
    }
};

class DFMProtocolDevice;
class DFMProtocolMonitorPrivate final : public DFMMonitorPrivate
{

public:
    DFMProtocolMonitorPrivate(DFMProtocolMonitor *qq);
    ~DFMProtocolMonitorPrivate();

    bool startMonitor() DFM_MNT_OVERRIDE;
    bool stopMonitor() DFM_MNT_OVERRIDE;
    DeviceType monitorObjectType() const DFM_MNT_OVERRIDE;
    QStringList getDevices() DFM_MNT_OVERRIDE;
    QSharedPointer<DFMDevice> createDevice(const QString &id) DFM_MNT_OVERRIDE;
    QSharedPointer<DFMDevice> createDeviceByMount();

private:
    void initDeviceList();

    static void onMountAdded(GVolumeMonitor *gVolMonitor, GMount *mount, gpointer userData);
    static void onMountChanged(GVolumeMonitor *gVolMonitor, GMount *mount, gpointer userData);
    static void onMountRemoved(GVolumeMonitor *gVolMonitor, GMount *mount, gpointer userData);
    static void onVolumeAdded(GVolumeMonitor *gVolMonitor, GVolume *volume, gpointer userData);
    static void onVolumeChanged(GVolumeMonitor *gVolMonitor, GVolume *volume, gpointer userData);
    static void onVolumeRemoved(GVolumeMonitor *gVolMonitor, GVolume *volume, gpointer userData);

    static bool hasDrive(GMount *mount);
    static bool hasDrive(GVolume *volume);

    QString findAssociatedMount(const QString &mpt);
    QString findOrphanVolume(const QString &volId);
    QStringList removeVolumes(const QString &volId);

    QMap<QString, DeviceCache> devices;
    QList<DFMProtocolDevice *> pdevices;

    void printDevices();

public:
    GVolumeMonitor *gVolMonitor { nullptr };
};

DFM_MOUNT_END_NS

#endif   // DFMProtocolMONITOR_P_H
