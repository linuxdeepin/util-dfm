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

#include "base/ddevicemonitor.h"
#include "base/dmount_global.h"
#include "private/ddevicemonitor_p.h"

#include <QDebug>

DFM_MOUNT_USE_NS

DFMMOUNT::DDeviceMonitorPrivate::DDeviceMonitorPrivate(DDeviceMonitor *qq)
    : q(qq)
{
}

DDeviceMonitorPrivate::~DDeviceMonitorPrivate()
{
}

DDeviceMonitor::DDeviceMonitor(DDeviceMonitorPrivate *dd, QObject *parent)
    : QObject(parent), d(dd)
{
}

DDeviceMonitor::~DDeviceMonitor()
{
}

bool DDeviceMonitor::startMonitor()
{
    Q_ASSERT_X(d->start, __PRETTY_FUNCTION__, "not register");

    d->monitorStatus = MonitorStatus::kMonitoring;
    return d->start();
}

bool DDeviceMonitor::stopMonitor()
{
    Q_ASSERT_X(d->stop, __PRETTY_FUNCTION__, "not register");

    d->monitorStatus = MonitorStatus::kIdle;
    return d->stop();
}

MonitorStatus DDeviceMonitor::status() const
{
    return d->monitorStatus;
}

DeviceType DDeviceMonitor::monitorObjectType() const
{
    Q_ASSERT_X(d->monitorObjectType, __PRETTY_FUNCTION__, "not register");

    return d->monitorObjectType();
}

QStringList DDeviceMonitor::getDevices() const
{
    Q_ASSERT_X(d->monitorObjectType, __PRETTY_FUNCTION__, "not register");

    return d->getDevices();
}

QSharedPointer<DDevice> DDeviceMonitor::createDeviceById(const QString &id) const
{
    Q_ASSERT_X(d->monitorObjectType, __PRETTY_FUNCTION__, "not register");

    return d->createDeviceById(id);
}

void DDeviceMonitor::registerStartMonitor(const DDeviceMonitor::StartMonitorFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->start = func;
}

void DDeviceMonitor::registerStopMonitor(const DDeviceMonitor::StopMonitorFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->stop = func;
}

void DDeviceMonitor::registerMonitorObjectType(const DDeviceMonitor::MonitorObjectTypeFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->monitorObjectType = func;
}

void DDeviceMonitor::registerGetDevices(const DDeviceMonitor::GetDevicesFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->getDevices = func;
}

void DDeviceMonitor::registerCreateDeviceById(const DDeviceMonitor::CreateDeviceByIdFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->createDeviceById = func;
}
