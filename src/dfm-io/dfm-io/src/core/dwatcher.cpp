// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/dwatcher.h"

#include "core/dwatcher_p.h"

USING_IO_NAMESPACE

DWatcherPrivate::DWatcherPrivate(DWatcher *q)
    : q(q)
{
}

DWatcherPrivate::~DWatcherPrivate()
{
}

DWatcher::DWatcher(const QUrl &uri, QObject *parent)
    : QObject(parent), d(new DWatcherPrivate(this))
{
    d->uri = uri;
}

DWatcher::~DWatcher()
{
}

QUrl DWatcher::uri() const
{
    return d->uri;
}

void DWatcher::setTimeRate(int msec)
{
    d->timeRate = msec;
}

int DWatcher::timeRate() const
{
    return d->timeRate;
}

void DWatcher::setWatchType(DWatcher::WatchType type)
{
    if (!d->setWatchTypeFunc)
        return;

    return d->setWatchTypeFunc(type);
}

DWatcher::WatchType DWatcher::watchType() const
{
    if (!d->watchTypeFunc)
        return DWatcher::WatchType::kAuto;

    return d->watchTypeFunc();
}

bool DWatcher::running() const
{
    if (!d->runningFunc)
        return false;

    return d->runningFunc();
}

bool DWatcher::start(int timeRate)
{
    if (!d->startFunc)
        return false;

    setTimeRate(timeRate);
    return d->startFunc(timeRate);
}

bool DWatcher::stop()
{
    if (!d->stopFunc)
        return false;

    return d->stopFunc();
}

void DWatcher::registerRunning(const DWatcher::RunningFunc &func)
{
    d->runningFunc = func;
}

void DWatcher::registerStart(const DWatcher::StartFunc &func)
{
    d->startFunc = func;
}

void DWatcher::registerStop(const DWatcher::StopFunc &func)
{
    d->stopFunc = func;
}

void DWatcher::registerSetWatchType(const DWatcher::SetWatchTypeFunc &func)
{
    d->setWatchTypeFunc = func;
}

void DWatcher::registerWatchType(const DWatcher::WatchTypeFunc &func)
{
    d->watchTypeFunc = func;
}

void DWatcher::registerLastError(const DWatcher::LastErrorFunc &func)
{
    d->lastErrorFunc = func;
}

DFMIOError DWatcher::lastError() const
{
    if (!d->lastErrorFunc)
        return DFMIOError();

    return d->lastErrorFunc();
}
