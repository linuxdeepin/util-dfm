// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDEVICEPRIVATE_H
#define DDEVICEPRIVATE_H

#include <dfm-mount/base/dmount_global.h>
#include <dfm-mount/base/ddevice.h>

DFM_MOUNT_BEGIN_NS

class CallbackProxy
{
public:
    CallbackProxy(DeviceOperateCallback cb)
        : cb(cb) { }
    CallbackProxy(DeviceOperateCallbackWithMessage cb)
        : cbWithInfo(cb) { }
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

    mutable OperationErrorInfo lastError { DeviceError::kNoError, "" };

protected:
    DDevice *q = nullptr;
};

DFM_MOUNT_END_NS

#endif   // DDEVICEPRIVATE_H
