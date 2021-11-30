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

typedef struct _GMount GMount;
typedef struct _GVolume GVolume;
typedef struct _GVolumeMonitor GVolumeMonitor;

DFM_MOUNT_BEGIN_NS

class DFMProtocolDevicePrivate;
class DFMProtocolDevice final : public DFMDevice
{
    Q_OBJECT
    friend class DFMProtocolMonitorPrivate;

public:
    ~DFMProtocolDevice();

    void setOperatorTimeout(int msecs);

private:
    DFMProtocolDevice(const QString &id, GVolume *vol, GMount *mnt, GVolumeMonitor *monitor, QObject *parent = nullptr);
    void setVolume(GVolume *);
    void setMount(GMount *);
};

DFM_MOUNT_END_NS

#endif   // DFMPROTOCOLDEVICE_H
