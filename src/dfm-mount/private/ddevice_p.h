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
#ifndef DDEVICEPRIVATE_H
#define DDEVICEPRIVATE_H

#include "base/dmount_global.h"
#include "base/ddevice.h"

DFM_MOUNT_BEGIN_NS

class CallbackProxy
{
public:
    CallbackProxy(DeviceOperateCallback cb)
        : cb(cb) {}
    CallbackProxy(DeviceOperateCallbackWithMessage cb)
        : cbWithInfo(cb) {}
    DeviceOperateCallback cb { nullptr };
    DeviceOperateCallbackWithMessage cbWithInfo { nullptr };
};

class DDevicePrivate
{
public:
    DDevicePrivate(DDevice *qq);
    virtual ~DDevicePrivate();

    DDevice::PathFunc path { nullptr };
    DDevice::MountFunc mount { nullptr };
    DDevice::MountAsyncFunc mountAsync { nullptr };
    DDevice::UnmountFunc unmount { nullptr };
    DDevice::UnmountAsyncFunc unmountAsync { nullptr };
    DDevice::RenameFunc rename { nullptr };
    DDevice::RenameAsyncFunc renameAsync { nullptr };
    DDevice::MountPointFunc mountPoint { nullptr };
    DDevice::FileSystemFunc fileSystem { nullptr };
    DDevice::SizeTotalFunc sizeTotal { nullptr };
    DDevice::SizeUsageFunc sizeUsage { nullptr };
    DDevice::SizeFreeFunc sizeFree { nullptr };
    DDevice::DeviceTypeFunc deviceType { nullptr };
    DDevice::GetPropertyFunc getProperty { nullptr };
    DDevice::DisplayNameFunc displayName { nullptr };

    mutable DeviceError lastError { DeviceError::kNoError };

protected:
    DDevice *q = nullptr;
};

DFM_MOUNT_END_NS

#endif   // DDEVICEPRIVATE_H
