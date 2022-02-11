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
#include "base/dfmmount_global.h"
#include "private/dfmmonitor_p.h"

DFM_MOUNT_USE_NS

DFMMOUNT::DFMMonitorPrivate::DFMMonitorPrivate(DFMMonitor *qq)
    : q(qq)
{
}

DFMMonitorPrivate::~DFMMonitorPrivate()
{
}

DFMMonitor::DFMMonitor(DFMMonitorPrivate *dd, QObject *parent)
    : QObject(parent), d(dd)
{
}

DFMMonitor::~DFMMonitor()
{
}

bool DFMMonitor::startMonitor()
{
    Q_ASSERT_X(d->start, __PRETTY_FUNCTION__, "not register");

    d->monitorStatus = MonitorStatus::Monitoring;
    return d->start();
}

bool DFMMonitor::stopMonitor()
{
    Q_ASSERT_X(d->stop, __PRETTY_FUNCTION__, "not register");

    d->monitorStatus = MonitorStatus::Idle;
    return d->stop();
}

MonitorStatus DFMMonitor::status() const
{
    return d->monitorStatus;
}

DeviceType DFMMonitor::monitorObjectType() const
{
    Q_ASSERT_X(d->monitorObjectType, __PRETTY_FUNCTION__, "not register");

    return d->monitorObjectType();
}

QStringList DFMMonitor::getDevices() const
{
    Q_ASSERT_X(d->monitorObjectType, __PRETTY_FUNCTION__, "not register");

    return d->getDevices();
}

QSharedPointer<DFMDevice> DFMMonitor::createDeviceById(const QString &id) const
{
    Q_ASSERT_X(d->monitorObjectType, __PRETTY_FUNCTION__, "not register");

    return d->createDeviceById(id);
}

void DFMMonitor::registerStartMonitor(const DFMMonitor::StartMonitorFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->start = func;
}

void DFMMonitor::registerStopMonitor(const DFMMonitor::StopMonitorFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->stop = func;
}

void DFMMonitor::registerMonitorObjectType(const DFMMonitor::MonitorObjectTypeFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->monitorObjectType = func;
}

void DFMMonitor::registerGetDevices(const DFMMonitor::GetDevicesFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->getDevices = func;
}

void DFMMonitor::registerCreateDeviceById(const DFMMonitor::CreateDeviceByIdFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->createDeviceById = func;
}
