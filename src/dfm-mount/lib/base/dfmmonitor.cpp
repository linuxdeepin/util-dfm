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
#include <QDebug>

#include "base/dfmmonitor.h"
#include "base/dfmmountdefines.h"
#include "private/dfmmonitor_p.h"

DFM_MOUNT_USE_NS

DFMMonitor::DFMMonitor(QObject *parent)
    : QObject(parent), d_ptr(new DFMMonitorPrivate(this))
{

}

bool DFMMonitor::startMonitor()
{
    Q_D(DFMMonitor);
    if (d->start)
        return d->start();
    WARN_NO_INIT();
    return false;
}

bool DFMMonitor::stopMonitor()
{
    Q_D(DFMMonitor);
    if (d->stop)
        return d->stop();
    WARN_NO_INIT();
    return false;
}

MonitorStatus DFMMonitor::status()
{
    Q_D(DFMMonitor);
    if (d->status)
        return d->status();
    WARN_NO_INIT();
    return NotDefined;
}

int DFMMonitor::monitorObjectType()
{
    Q_D(DFMMonitor);
    if (d->mot)
        return d->mot();
    WARN_NO_INIT();
    return -1;
}

void DFMMonitor::registerStartMonitor(const DFMMonitor::StartMonitor &func)
{
    Q_D(DFMMonitor);
    if (func)
        d->start = func;
}

void DFMMonitor::registerStopMonitor(const DFMMonitor::StopMonitor &func)
{
    Q_D(DFMMonitor);
    if (func)
        d->stop = func;
}

void DFMMonitor::registerStatus(const DFMMonitor::Status &func)
{
    Q_D(DFMMonitor);
    if (func)
        d->status = func;
}

void DFMMonitor::registerMonitorObjectType(const DFMMonitor::MonitorObjectType &func)
{
    Q_D(DFMMonitor);
    if (func)
        d->mot = func;
}

DFMMonitorPrivate::DFMMonitorPrivate(DFMMonitor *q)
    : q_ptr(q)
{

}
