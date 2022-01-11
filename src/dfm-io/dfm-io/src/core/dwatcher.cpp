/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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

void DWatcher::setWatchAttributeIDList(const QList<DFileInfo::AttributeID> &ids)
{
    d->ids = ids;
}

void DWatcher::addWatchAttributeID(const DFileInfo::AttributeID &id)
{
    d->ids.push_back(id);
}

QList<DFileInfo::AttributeID> DWatcher::watchAttributeIDList() const
{
    return d->ids;
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
