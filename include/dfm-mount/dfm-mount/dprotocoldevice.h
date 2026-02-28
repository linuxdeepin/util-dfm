// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DPROTOCOLDEVICE_H
#define DPROTOCOLDEVICE_H

#include <dfm-mount/base/ddevice.h>
#include <dfm-mount/dprotocolmonitor.h>

#include <QObject>

typedef struct _GVolumeMonitor GVolumeMonitor;

DFM_MOUNT_BEGIN_NS

struct MountPassInfo
{
    QString userName {};
    QString passwd {};
    QString domain {};
    bool anonymous { false };
    bool cancelled { false };
    int timeout = 0;
    NetworkMountPasswdSaveMode savePasswd { NetworkMountPasswdSaveMode::kNeverSavePasswd };
};

using GetMountPassInfo = std::function<MountPassInfo(
        const QString &message, const QString &userDefault, const QString &domainDefault)>;
using GetUserChoice = std::function<int(const QString &message, const QStringList &choices)>;

class DProtocolDevicePrivate;
class DProtocolDevice final : public DDevice
{
    Q_OBJECT
    friend class DProtocolMonitorPrivate;

public:
    ~DProtocolDevice();
    QStringList deviceIcons() const;
    static void mountNetworkDevice(const QString &address, GetMountPassInfo getPassInfo,
                                   GetUserChoice getUserChoice,
                                   DeviceOperateCallbackWithMessage mountResult, int secs = 0);

    static bool isMountByDaemon(const QString &address);

    void setOperatorTimeout(int msecs);

private:
    DProtocolDevice(const QString &id, GVolumeMonitor *monitor, QObject *parent = nullptr);

private Q_SLOTS:
    void mounted(const QString &id);
    void unmounted(const QString &id);
};

DFM_MOUNT_END_NS

#endif   // DPROTOCOLDEVICE_H
