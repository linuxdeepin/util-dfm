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
#ifndef DFMMONITORPRIVATE_H
#define DFMMONITORPRIVATE_H

#include "base/dfmmount_global.h"
#include "base/dfmmonitor.h"

#include <private/qobject_p.h>

#include <functional>

using namespace std;
DFM_MOUNT_BEGIN_NS

class DFMMonitorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(DFMMonitor)
public:
    DFMMonitorPrivate();

    DFMMonitor::StartMonitorFunc start = nullptr;
    DFMMonitor::StopMonitorFunc stop = nullptr;
    DFMMonitor::StatusFunc status = nullptr;
    DFMMonitor::MonitorObjectTypeFunc mot = nullptr;
    DFMMonitor::GetDevicesFunc getDevices = nullptr;
};

DFM_MOUNT_END_NS

#endif // DFMMONITORPRIVATE_H
