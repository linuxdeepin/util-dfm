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

#include "dfmblockmonitor.h"
#include "private/dfmblockmonitor_p.h"

DFM_MOUNT_USE_NS

DFMBlockMonitor::DFMBlockMonitor(QObject *parent)
    :DFMAbstractMonitor (parent)
{

}

DFMBlockMonitor::~DFMBlockMonitor()
{

}

DFMBlockMonitorPrivate::DFMBlockMonitorPrivate(DFMBlockMonitor *qq)
    :q_ptr(qq)
{
    q_ptr->iface.startMonitor = std::bind(&DFMBlockMonitorPrivate::startMonitor, this);
    q_ptr->iface.stopMonitor = std::bind(&DFMBlockMonitorPrivate::stopMonitor, this);
    q_ptr->iface.status = std::bind(&DFMBlockMonitorPrivate::status, this);
    q_ptr->iface.monitorObjectType = std::bind(&DFMBlockMonitorPrivate::monitorObjectType, this);
}

bool DFMBlockMonitorPrivate::startMonitor()
{
    return false;
}

bool DFMBlockMonitorPrivate::stopMonitor()
{
    return false;
}

MonitorStatus DFMBlockMonitorPrivate::status()
{
    return MonitorStatus::Idle;
}

int DFMBlockMonitorPrivate::monitorObjectType()
{
    return BlockDevice;
}
