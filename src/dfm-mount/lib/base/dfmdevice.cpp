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

DFMDevice::DFMDevice(QObject *parent)
    : QObject (parent), d_ptr(new DFMDevicePrivate(this))
{

}

QString DFMDevice::path()
{
    Q_D(DFMDevice);
    if (d->path)
        return d->path();
    WARN_NO_INIT();
    return QString();
}

QUrl DFMDevice::mount(const QVariantMap &opts)
{
    Q_D(DFMDevice);
    if (d->mount)
        return d->mount(opts);
    WARN_NO_INIT();
    return QUrl();
}

void DFMDevice::mountAsync(const QVariantMap &opts)
{
    Q_D(DFMDevice);
    if (d->mountAsync)
        return d->mountAsync(opts);
    WARN_NO_INIT();
}

bool DFMDevice::unmount()
{
    Q_D(DFMDevice);
    if (d->unmount)
        return d->unmount();
    WARN_NO_INIT();
    return false;
}

void DFMDevice::unmountAsync()
{
    Q_D(DFMDevice);
    if (d->unmountAsync)
        return d->unmountAsync();
    WARN_NO_INIT();
}

bool DFMDevice::rename(const QString &newName)
{
    Q_D(DFMDevice);
    if (d->rename)
        return d->rename(newName);
    WARN_NO_INIT();
    return false;
}

void DFMDevice::renameAsync(const QString &newName)
{
    Q_D(DFMDevice);
    if (d->renameAsync)
        return d->renameAsync(newName);
    WARN_NO_INIT();
}

QUrl DFMDevice::accessPoint()
{
    Q_D(DFMDevice);
    if (d->accessPoint)
        return d->accessPoint();
    WARN_NO_INIT();
    return QUrl();
}

QUrl DFMDevice::mountPoint()
{
    Q_D(DFMDevice);
    if (d->mountPoint)
        return d->mountPoint();
    WARN_NO_INIT();
    return QUrl();
}

QString DFMDevice::fileSystem()
{
    Q_D(DFMDevice);
    if (d->fileSystem)
        return d->fileSystem();
    WARN_NO_INIT();
    return QString();
}

long DFMDevice::sizeTotal()
{
    Q_D(DFMDevice);
    if (d->sizeTotal)
        return d->sizeTotal();
    WARN_NO_INIT();
    return 0;
}

long DFMDevice::sizeFree()
{
    Q_D(DFMDevice);
    if (d->sizeFree)
        return d->sizeFree();
    WARN_NO_INIT();
    return 0;
}

long DFMDevice::sizeUsage()
{
    Q_D(DFMDevice);
    if (d->sizeUsage)
        return d->sizeUsage();
    WARN_NO_INIT();
    return 0;
}

int DFMDevice::deviceType()
{
    Q_D(DFMDevice);
    if (d->deviceType)
        return d->deviceType();
    WARN_NO_INIT();
    return 0;
}

QVariant DFMDevice::getProperty(Property item)
{
    Q_D(DFMDevice);
    if (d->getProperty)
        return d->getProperty(item);
    WARN_NO_INIT();
    return QVariant();
}

void DFMDevice::registerPath(const DFMDevice::Path &func)
{
    Q_D(DFMDevice);
    if (func)
        d->path = func;
}

void DFMDevice::registerMount(const DFMDevice::Mount &func)
{
    Q_D(DFMDevice);
    if (func)
        d->mount = func;
}

void DFMDevice::registerMountAsync(const DFMDevice::MountAsync &func)
{
    Q_D(DFMDevice);
    if (func)
        d->mountAsync = func;
}

void DFMDevice::registerUnmount(const DFMDevice::Unmount &func)
{
    Q_D(DFMDevice);
    if (func)
        d->unmount = func;
}

void DFMDevice::registerUnmountAsync(const DFMDevice::UnmountAsync &func)
{
    Q_D(DFMDevice);
    if (func)
        d->unmountAsync = func;
}

void DFMDevice::registerRename(const DFMDevice::Rename &func)
{
    Q_D(DFMDevice);
    if (func)
        d->rename = func;
}

void DFMDevice::registerRenameAsync(const DFMDevice::RenameAsync &func)
{
    Q_D(DFMDevice);
    if (func)
        d->renameAsync = func;
}

void DFMDevice::registerAccessPoint(const DFMDevice::AccessPoint &func)
{
    Q_D(DFMDevice);
    if (func)
        d->accessPoint = func;
}

void DFMDevice::registerMountPoint(const DFMDevice::MountPoint &func)
{
    Q_D(DFMDevice);
    if (func)
        d->mountPoint = func;
}

void DFMDevice::registerFileSystem(const DFMDevice::FileSystem &func)
{
    Q_D(DFMDevice);
    if (func)
        d->fileSystem = func;
}

void DFMDevice::registerSizeTotal(const DFMDevice::SizeTotal &func)
{
    Q_D(DFMDevice);
    if (func)
        d->sizeTotal = func;
}

void DFMDevice::registerSizeUsage(const DFMDevice::SizeUsage &func)
{
    Q_D(DFMDevice);
    if (func)
        d->sizeUsage = func;
}

void DFMDevice::registerSizeFree(const DFMDevice::SizeFree &func)
{
    Q_D(DFMDevice);
    if (func)
        d->sizeFree = func;
}

void DFMDevice::registerDeviceType(const DFMDevice::DeviceType &func)
{
    Q_D(DFMDevice);
    if (func)
        d->deviceType = func;
}

void DFMDevice::registerGetProperty(const DFMDevice::GetProperty &func)
{
    Q_D(DFMDevice);
    if (func)
        d->getProperty = func;
}

dfm_mount::DFMDevicePrivate::DFMDevicePrivate(DFMDevice *q)
    : q_ptr(q)
{

}
