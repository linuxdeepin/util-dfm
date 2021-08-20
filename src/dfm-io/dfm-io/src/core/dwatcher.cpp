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
    : q_ptr(q)
{

}

DWatcherPrivate::~DWatcherPrivate()
{

}

DWatcher::DWatcher(const QUrl &uri, QObject *parent)
    : QObject(parent)
    , d_ptr(new DWatcherPrivate(this))
{
    Q_D(DWatcher);

    d->uri = uri;
}

DWatcher::~DWatcher()
{
    
}

QUrl DWatcher::uri() const
{
    Q_D(const DWatcher);

    return d->uri;
}

void DWatcher::setTimeRate(int msec)
{
    Q_D(DWatcher);

    d->timeRate = msec;
}

int DWatcher::timeRate() const
{
    Q_D(const DWatcher);

    return d->timeRate;
}

void DWatcher::setWatchAttributeIDList(const QList<DFileInfo::AttributeID> &ids)
{
    Q_D(DWatcher);

    d->ids = ids;
}

void DWatcher::addWatchAttributeID(const DFileInfo::AttributeID &id)
{
    Q_D(DWatcher);

    d->ids.push_back(id);
}

QList<DFileInfo::AttributeID> DWatcher::watchAttributeIDList() const
{
    Q_D(const DWatcher);

    return d->ids;
}

bool DWatcher::running() const
{
    Q_D(const DWatcher);

    return false;
}

DFM_VIRTUAL bool DWatcher::start(int timeRate)
{
    Q_D(DWatcher);

    if (!d->startFunc)
        return false;

    setTimeRate(timeRate);
    return d->startFunc(timeRate);
}

DFM_VIRTUAL bool DWatcher::stop()
{
    Q_D(DWatcher);

    if (!d->stopFunc)
        return false;

    return d->stopFunc();
}

void DWatcher::registerStart(const DWatcher::StartFunc &func)
{
    Q_D(DWatcher);

    d->startFunc = func;
}

void DWatcher::registerStop(const DWatcher::StopFunc &func)
{
    Q_D(DWatcher);

    d->stopFunc = func;
}

DFMIOError DWatcher::lastError() const
{
    Q_D(const DWatcher);

    if (!d)
        return DFMIOError();

    return d->error;
}
