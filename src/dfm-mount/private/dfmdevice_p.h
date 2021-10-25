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

class DFMDevicePrivate
{
public:
    DFMDevicePrivate(DFMDevice *qq);
    virtual ~DFMDevicePrivate();

    DFMDevice::PathFunc path = nullptr;
    DFMDevice::MountFunc mount = nullptr;
    DFMDevice::MountAsyncFunc mountAsync = nullptr;
    DFMDevice::UnmountFunc unmount = nullptr;
    DFMDevice::UnmountAsyncFunc unmountAsync = nullptr;
    DFMDevice::RenameFunc rename = nullptr;
    DFMDevice::RenameAsyncFunc renameAsync = nullptr;
    DFMDevice::MountPointFunc mountPoint = nullptr;
    DFMDevice::FileSystemFunc fileSystem = nullptr;
    DFMDevice::SizeTotalFunc sizeTotal = nullptr;
    DFMDevice::SizeUsageFunc sizeUsage = nullptr;
    DFMDevice::SizeFreeFunc sizeFree = nullptr;
    DFMDevice::DeviceTypeFunc deviceType = nullptr;
    DFMDevice::GetPropertyFunc getProperty = nullptr;

    MountError lastError;
protected:
    DFMDevice *q = nullptr;
};

DFM_MOUNT_END_NS

#endif // DFMDEVICEPRIVATE_H
