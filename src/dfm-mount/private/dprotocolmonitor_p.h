// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DPROTOCOLMONITOR_P_H
#define DPROTOCOLMONITOR_P_H

#include <dfm-mount/dprotocolmonitor.h>

#include "private/ddevicemonitor_p.h"

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

class DProtocolMonitorPrivate final : public DDeviceMonitorPrivate
{

public:
    DProtocolMonitorPrivate(DProtocolMonitor *qq);
    ~DProtocolMonitorPrivate();

    bool startMonitor() DMNT_OVERRIDE;
    bool stopMonitor() DMNT_OVERRIDE;
    DeviceType monitorObjectType() const DMNT_OVERRIDE;
    QStringList getDevices() DMNT_OVERRIDE;
    QSharedPointer<DDevice> createDevice(const QString &id) DMNT_OVERRIDE;
    QSharedPointer<DDevice> createDeviceByMount();

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
    static bool isNativeMount(const QString &mpt);
    static bool isMountByOther(const QString &mpt);

    static bool isOrphanMount(GMount *mount);

    QSet<QString> cachedDevices;

public:
    GVolumeMonitor *gVolMonitor { nullptr };
};

DFM_MOUNT_END_NS

#endif   // DFMProtocolMONITOR_P_H
