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
#ifndef DFMDEVICEPRIVATE_H
#define DFMDEVICEPRIVATE_H

#include "base/dfmmount_global.h"
#include "base/dfmdevice.h"

#include <QObject>
#include <QUrl>

#include <functional>

using namespace std;
DFM_MOUNT_BEGIN_NS

class DFMDevicePrivate {
public:
    DFMDevicePrivate(DFMDevice *q);

    DFMDevice::Path path = nullptr;
    DFMDevice::Mount mount = nullptr;
    DFMDevice::MountAsync mountAsync = nullptr;
    DFMDevice::Unmount unmount = nullptr;
    DFMDevice::UnmountAsync unmountAsync = nullptr;
    DFMDevice::Rename rename = nullptr;
    DFMDevice::RenameAsync renameAsync = nullptr;
    DFMDevice::AccessPoint accessPoint = nullptr;
    DFMDevice::MountPoint mountPoint = nullptr;
    DFMDevice::FileSystem fileSystem = nullptr;
    DFMDevice::SizeTotal sizeTotal = nullptr;
    DFMDevice::SizeUsage sizeUsage = nullptr;
    DFMDevice::SizeFree sizeFree = nullptr;
    DFMDevice::DeviceType deviceType = nullptr;
    DFMDevice::GetProperty getProperty = nullptr;

public:
    DFMDevice *q_ptr = nullptr;
};

DFM_MOUNT_END_NS

#endif // DFMDEVICEPRIVATE_H
