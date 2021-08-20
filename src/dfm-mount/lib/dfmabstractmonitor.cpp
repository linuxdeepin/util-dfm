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

#include "dfmabstractmonitor.h"
#include "dfmmountdefines.h"

DFM_MOUNT_USE_NS

DFMAbstractMonitor::DFMAbstractMonitor(QObject *parent)
    :QObject(parent)
{

}

bool DFMAbstractMonitor::startMonitor()
{
    if (iface.startMonitor)
        return iface.startMonitor();
    WARN_NO_INIT();
    return false;
}

bool DFMAbstractMonitor::stopMonitor()
{
    if (iface.stopMonitor)
        return iface.stopMonitor();
    WARN_NO_INIT();
    return false;
}

MonitorStatus DFMAbstractMonitor::status()
{
    if (iface.status)
        return iface.status();
    WARN_NO_INIT();
    return NotDefined;
}

int DFMAbstractMonitor::monitorObjectType()
{
    if (iface.monitorObjectType)
        return iface.monitorObjectType();
    WARN_NO_INIT();
    return -1;
}
