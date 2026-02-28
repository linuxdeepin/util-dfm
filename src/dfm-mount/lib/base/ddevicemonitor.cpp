// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-mount/base/ddevicemonitor.h>
#include <dfm-mount/base/dmount_global.h>

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
