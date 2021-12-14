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
#ifndef DFMPROTOCOLDEVICE_H
#define DFMPROTOCOLDEVICE_H

#include "base/dfmdevice.h"
#include "dfmprotocolmonitor.h"

#include <QObject>

typedef struct _GVolumeMonitor GVolumeMonitor;

DFM_MOUNT_BEGIN_NS

struct MountPassInfo
{
    QString userName {};
    QString passwd {};
    QString domain {};
    bool anonymous { false };
    int savePasswd { 0 };   // 0 for never save, 1 for current session, and 2 for save permanently
};

//typedef struct _GVolumeMonitor GVolumeMonitor;
typedef MountPassInfo (*GetMountPassInfo)(const QString &message, const QString &userDefault, const QString &domainDefault, const QString &errMsg);
typedef void (*MountResult)(bool ok, QString errMsg);

class DFMProtocolDevicePrivate;
class DFMProtocolDevice final : public DFMDevice
{
    Q_OBJECT
    friend class DFMProtocolMonitorPrivate;

public:
    ~DFMProtocolDevice();
    static void mountNetworkDevice(const QString &address, GetMountPassInfo getPassInfo, MountResult mountResult);

    void setOperatorTimeout(int msecs);

private:
    DFMProtocolDevice(const QString &id, GVolumeMonitor *monitor, QObject *parent = nullptr);

private Q_SLOTS:
    void mounted(const QString &id);
    void unmounted(const QString &id);
};

DFM_MOUNT_END_NS

#endif   // DFMPROTOCOLDEVICE_H
