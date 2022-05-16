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
#ifndef DPROTOCOLDEVICE_H
#define DPROTOCOLDEVICE_H

#include "base/ddevice.h"
#include "dprotocolmonitor.h"

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
    NetworkMountPasswdSaveMode savePasswd { NetworkMountPasswdSaveMode::kNeverSavePasswd };
};

using GetMountPassInfo = std::function<MountPassInfo(const QString &message, const QString &userDefault, const QString &domainDefault)>;
using GetUserChoice = std::function<int(const QString &message, const QStringList &choices)>;

class DProtocolDevicePrivate;
class DProtocolDevice final : public DDevice
{
    Q_OBJECT
    friend class DProtocolMonitorPrivate;

public:
    ~DProtocolDevice();
    QStringList deviceIcons() const;
    static void mountNetworkDevice(const QString &address, GetMountPassInfo getPassInfo, GetUserChoice getUserChoice, DeviceOperateCallbackWithMessage mountResult, int msecs = 0);

    void setOperatorTimeout(int msecs);

private:
    DProtocolDevice(const QString &id, GVolumeMonitor *monitor, QObject *parent = nullptr);

private Q_SLOTS:
    void mounted(const QString &id);
    void unmounted(const QString &id);
};

DFM_MOUNT_END_NS

#endif   // DPROTOCOLDEVICE_H
