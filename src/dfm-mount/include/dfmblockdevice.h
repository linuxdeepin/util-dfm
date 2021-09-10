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
#ifndef DFMBLOCKDEVICE_H
#define DFMBLOCKDEVICE_H

#include "base/dfmdevice.h"

#include <QObject>

DFM_MOUNT_BEGIN_NS

class DFMBlockDevicePrivate;
class DFMBlockDevice final : public DFMDevice
{
    Q_OBJECT

public:
    DFMBlockDevice() = delete;
    DFMBlockDevice(const DFMBlockDevice &) = delete;
    ~DFMBlockDevice();

    QString deviceDescriptor() const;

private:
    DFMBlockDevice(QObject *parent = nullptr);

public:
    bool eject();
    bool powerOff();

    friend class DFMBlockMonitorPrivate;
};

DFM_MOUNT_END_NS

#endif // DFMBLOCKDEVICE_H
