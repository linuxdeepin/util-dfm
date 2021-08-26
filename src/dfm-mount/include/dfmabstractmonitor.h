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
#ifndef DFMABSTRACTMONITOR_H
#define DFMABSTRACTMONITOR_H

#include "dfmmount_global.h"
#include "dfmmonitorinterface.h"

#include <QObject>

DFM_MOUNT_BEGIN_NS
class DFMAbstractDevice;
class DFMAbstractMonitor: public QObject
{
    Q_OBJECT
public:
    DFMAbstractMonitor(QObject *parent);
    bool startMonitor();
    bool stopMonitor();
    MonitorStatus status();
    int monitorObjectType();

Q_SIGNALS:
    void driveAdded();
    void driveRemoved();
    void deviceAdded(DFMAbstractDevice *dev);
    void deviceRemoved(DFMAbstractDevice *dev);
    void mountAdded(const QString &mountPoint);
    void mountRemoved(const QString &mountPoint);
    void propertiesChanged(const QVariantMap &);

protected:
    DfmMonitorInterface iface;
};
DFM_MOUNT_END_NS

#endif // DFMABSTRACTMONITOR_H
