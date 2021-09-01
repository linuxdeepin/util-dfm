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

#include <QDebug>

DFM_MOUNT_USE_NS

DFMProtocolMonitor::DFMProtocolMonitor(QObject *parent)
    : DFMMonitor (* new DFMProtocolMonitorPrivate(), parent)
{
    Q_D(DFMProtocolMonitor);
    registerStartMonitor(std::bind(&DFMProtocolMonitorPrivate::startMonitor, d));
    registerStopMonitor(std::bind(&DFMProtocolMonitorPrivate::stopMonitor, d));
    registerStatus(std::bind(&DFMProtocolMonitorPrivate::status, d));
    registerMonitorObjectType(std::bind(&DFMProtocolMonitorPrivate::monitorObjectType, d));
}

DFMProtocolMonitor::~DFMProtocolMonitor()
{

}

DFMProtocolMonitorPrivate::DFMProtocolMonitorPrivate()
{

}

bool DFMProtocolMonitorPrivate::startMonitor()
{

    return true;
}

bool DFMProtocolMonitorPrivate::stopMonitor()
{

    return true;
}

MonitorStatus DFMProtocolMonitorPrivate::status() const
{
    return curStatus;
}

DeviceType DFMProtocolMonitorPrivate::monitorObjectType() const
{
    return DeviceType::ProtocolDevice;
}

