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

#include <QMap>

extern "C" {
#include <udisks/udisks.h>
}

DFM_MOUNT_BEGIN_NS
class DFMProtocolDevice;
class DFMProtocolMonitorPrivate {
public:
    DFMProtocolMonitorPrivate(DFMProtocolMonitor *qq);
    bool startMonitor();
    bool stopMonitor();
    MonitorStatus status();
    int monitorObjectType();

private:

public:
    MonitorStatus curStatus = MonitorStatus::Idle;

    QMap<QString, DFMProtocolDevice *> devices;

    DFMProtocolMonitor *q_ptr;
    Q_DECLARE_PUBLIC(DFMProtocolMonitor)
};

DFM_MOUNT_END_NS

#endif // DFMProtocolMONITOR_P_H
