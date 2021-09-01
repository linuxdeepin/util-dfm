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

#include "base/dfmmountdefines.h"
#include "base/dfmdevice.h"
#include "private/dfmdevice_p.h"

#include <QDebug>

DFM_MOUNT_USE_NS

DFM_MOUNT_NAMESPACE::DFMDevicePrivate::DFMDevicePrivate()
{

}

DFMDevice::DFMDevice(QObject *parent)
    : QObject (* new DFMDevicePrivate, parent)
{

}

QString DFMDevice::path() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->path, __PRETTY_FUNCTION__, "not register");

    return d->path();
}

QUrl DFMDevice::mount(const QVariantMap &opts)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(d->mount, __PRETTY_FUNCTION__, "not register");

    return d->mount(opts);
}

void DFMDevice::mountAsync(const QVariantMap &opts)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(d->mountAsync, __PRETTY_FUNCTION__, "not register");

    return d->mountAsync(opts);
}

bool DFMDevice::unmount()
{
    Q_D(DFMDevice);
    Q_ASSERT_X(d->unmount, __PRETTY_FUNCTION__, "not register");

    return d->unmount();
}

void DFMDevice::unmountAsync()
{
    Q_D(DFMDevice);
    Q_ASSERT_X(d->unmountAsync, __PRETTY_FUNCTION__, "not register");

    return d->unmountAsync();
}

bool DFMDevice::rename(const QString &newName)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(d->rename, __PRETTY_FUNCTION__, "not register");

    return d->rename(newName);
}

void DFMDevice::renameAsync(const QString &newName)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(d->renameAsync, __PRETTY_FUNCTION__, "not register");

    return d->renameAsync(newName);
}

QUrl DFMDevice::accessPoint() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->accessPoint, __PRETTY_FUNCTION__, "not register");

    return d->accessPoint();
}

QUrl DFMDevice::mountPoint() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->mountPoint, __PRETTY_FUNCTION__, "not register");

    return d->mountPoint();
}

QString DFMDevice::fileSystem() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->fileSystem, __PRETTY_FUNCTION__, "not register");

    return d->fileSystem();
}

long DFMDevice::sizeTotal() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->sizeTotal, __PRETTY_FUNCTION__, "not register");

    return d->sizeTotal();
}

long DFMDevice::sizeFree() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->sizeFree, __PRETTY_FUNCTION__, "not register");

    return d->sizeFree();
}

long DFMDevice::sizeUsage() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->sizeUsage, __PRETTY_FUNCTION__, "not register");

    return d->sizeUsage();
}

DeviceType DFMDevice::deviceType() const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->deviceType, __PRETTY_FUNCTION__, "not register");

    return d->deviceType();
}

QVariant DFMDevice::getProperty(Property item) const
{
    Q_D(const DFMDevice);
    Q_ASSERT_X(d->getProperty, __PRETTY_FUNCTION__, "not register");

    return d->getProperty(item);
}

void DFMDevice::registerPath(const DFMDevice::PathFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->path = func;
}

void DFMDevice::registerMount(const DFMDevice::MountFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->mount = func;
}

void DFMDevice::registerMountAsync(const DFMDevice::MountAsyncFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->mountAsync = func;
}

void DFMDevice::registerUnmount(const DFMDevice::UnmountFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->unmount = func;
}

void DFMDevice::registerUnmountAsync(const DFMDevice::UnmountAsyncFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->unmountAsync = func;
}

void DFMDevice::registerRename(const DFMDevice::RenameFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->rename = func;
}

void DFMDevice::registerRenameAsync(const DFMDevice::RenameAsyncFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->renameAsync = func;
}

void DFMDevice::registerAccessPoint(const DFMDevice::AccessPointFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->accessPoint = func;
}

void DFMDevice::registerMountPoint(const DFMDevice::MountPointFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->mountPoint = func;
}

void DFMDevice::registerFileSystem(const DFMDevice::FileSystemFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->fileSystem = func;
}

void DFMDevice::registerSizeTotal(const DFMDevice::SizeTotalFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->sizeTotal = func;
}

void DFMDevice::registerSizeUsage(const DFMDevice::SizeUsageFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->sizeUsage = func;
}

void DFMDevice::registerSizeFree(const DFMDevice::SizeFreeFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->sizeFree = func;
}

void DFMDevice::registerDeviceType(const DFMDevice::DeviceTypeFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->deviceType = func;
}

void DFMDevice::registerGetProperty(const DFMDevice::GetPropertyFunc &func)
{
    Q_D(DFMDevice);
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->getProperty = func;
}

DFMDevice::DFMDevice(DFMDevicePrivate &dd, QObject *parent)
    : QObject (dd, parent)
{

}

