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

#include "base/dmount_global.h"
#include "base/ddevice.h"
#include "private/ddevice_p.h"

#include <QDebug>

DFM_MOUNT_USE_NS

DFMMOUNT::DDevicePrivate::DDevicePrivate(DDevice *qq)
    : q(qq)
{
}

DDevicePrivate::~DDevicePrivate()
{
}

DDevice::DDevice(DDevicePrivate *dd, QObject *parent)
    : QObject(parent), d(dd)
{
}

DDevice::~DDevice()
{
}

QString DDevice::path() const
{
    Q_ASSERT_X(d->path, __PRETTY_FUNCTION__, "not register");

    return d->path();
}

QString DDevice::mount(const QVariantMap &opts)
{
    Q_ASSERT_X(d->mount, __PRETTY_FUNCTION__, "not register");

    return d->mount(opts);
}

void DDevice::mountAsync(const QVariantMap &opts, DeviceOperateCallbackWithMessage cb)
{
    Q_ASSERT_X(d->mountAsync, __PRETTY_FUNCTION__, "not register");

    return d->mountAsync(opts, cb);
}

bool DDevice::unmount(const QVariantMap &opts)
{
    Q_ASSERT_X(d->unmount, __PRETTY_FUNCTION__, "not register");

    return d->unmount(opts);
}

void DDevice::unmountAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    Q_ASSERT_X(d->unmountAsync, __PRETTY_FUNCTION__, "not register");

    return d->unmountAsync(opts, cb);
}

bool DDevice::rename(const QString &newName, const QVariantMap &opts)
{
    Q_ASSERT_X(d->rename, __PRETTY_FUNCTION__, "not register");

    return d->rename(newName, opts);
}

void DDevice::renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCallback cb)
{
    Q_ASSERT_X(d->renameAsync, __PRETTY_FUNCTION__, "not register");

    return d->renameAsync(newName, opts, cb);
}

QString DDevice::mountPoint() const
{
    Q_ASSERT_X(d->mountPoint, __PRETTY_FUNCTION__, "not register");

    return d->mountPoint();
}

QString DDevice::fileSystem() const
{
    Q_ASSERT_X(d->fileSystem, __PRETTY_FUNCTION__, "not register");

    return d->fileSystem();
}

qint64 DDevice::sizeTotal() const
{
    Q_ASSERT_X(d->sizeTotal, __PRETTY_FUNCTION__, "not register");

    return d->sizeTotal();
}

qint64 DDevice::sizeFree() const
{
    Q_ASSERT_X(d->sizeFree, __PRETTY_FUNCTION__, "not register");

    return d->sizeFree();
}

qint64 DDevice::sizeUsage() const
{
    Q_ASSERT_X(d->sizeUsage, __PRETTY_FUNCTION__, "not register");

    return d->sizeUsage();
}

DeviceType DDevice::deviceType() const
{
    Q_ASSERT_X(d->deviceType, __PRETTY_FUNCTION__, "not register");

    return d->deviceType();
}

QVariant DDevice::getProperty(Property item) const
{
    Q_ASSERT_X(d->getProperty, __PRETTY_FUNCTION__, "not register");

    return d->getProperty(item);
}

QString DDevice::displayName() const
{
    Q_ASSERT_X(d->displayName, __PRETTY_FUNCTION__, "not register");

    return d->displayName();
}

DeviceError DDevice::lastError() const
{
    return d->lastError;
}

void DDevice::registerPath(const DDevice::PathFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->path = func;
}

void DDevice::registerMount(const DDevice::MountFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->mount = func;
}

void DDevice::registerMountAsync(const DDevice::MountAsyncFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->mountAsync = func;
}

void DDevice::registerUnmount(const DDevice::UnmountFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->unmount = func;
}

void DDevice::registerUnmountAsync(const DDevice::UnmountAsyncFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->unmountAsync = func;
}

void DDevice::registerRename(const DDevice::RenameFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->rename = func;
}

void DDevice::registerRenameAsync(const DDevice::RenameAsyncFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->renameAsync = func;
}

void DDevice::registerMountPoint(const DDevice::MountPointFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->mountPoint = func;
}

void DDevice::registerFileSystem(const DDevice::FileSystemFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->fileSystem = func;
}

void DDevice::registerSizeTotal(const DDevice::SizeTotalFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->sizeTotal = func;
}

void DDevice::registerSizeUsage(const DDevice::SizeUsageFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->sizeUsage = func;
}

void DDevice::registerSizeFree(const DDevice::SizeFreeFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->sizeFree = func;
}

void DDevice::registerDeviceType(const DDevice::DeviceTypeFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->deviceType = func;
}

void DDevice::registerGetProperty(const DDevice::GetPropertyFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->getProperty = func;
}

void DDevice::registerDisplayName(const DDevice::DisplayNameFunc &func)
{
    Q_ASSERT_X(func, __PRETTY_FUNCTION__, "not register");

    d->displayName = func;
}
