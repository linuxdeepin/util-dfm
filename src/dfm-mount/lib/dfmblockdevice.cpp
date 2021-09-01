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

#include "dfmblockdevice.h"
#include "private/dfmblockdevice_p.h"

#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include <functional>
extern "C" {
#include <udisks/udisks.h>
}

DFM_MOUNT_USE_NS

DFMBlockDevice::DFMBlockDevice(const QString &device, QObject *parent)
    : DFMDevice(* new DFMBlockDevicePrivate(device), parent)
{
    Q_D(DFMBlockDevice);
    registerPath(std::bind(&DFMBlockDevicePrivate::path, d));
    registerMount(std::bind(&DFMBlockDevicePrivate::mount, d, std::placeholders::_1));
    registerMountAsync(std::bind(&DFMBlockDevicePrivate::mountAsync, d, std::placeholders::_1));
    registerUnmount(std::bind(&DFMBlockDevicePrivate::unmount, d));
    registerUnmountAsync(std::bind(&DFMBlockDevicePrivate::unmountAsync, d));
    registerRename(std::bind(&DFMBlockDevicePrivate::rename, d, std::placeholders::_1));
    registerRenameAsync(std::bind(&DFMBlockDevicePrivate::renameAsync, d, std::placeholders::_1));
    registerAccessPoint(std::bind(&DFMBlockDevicePrivate::accessPoint, d));
    registerMountPoint(std::bind(&DFMBlockDevicePrivate::mountPoint, d));
    registerFileSystem(std::bind(&DFMBlockDevicePrivate::fileSystem, d));
    registerSizeTotal(std::bind(&DFMBlockDevicePrivate::sizeTotal, d));
    registerSizeUsage(std::bind(&DFMBlockDevicePrivate::sizeUsage, d));
    registerSizeFree(std::bind(&DFMBlockDevicePrivate::sizeFree, d));
    registerDeviceType(std::bind(&DFMBlockDevicePrivate::deviceType, d));
}

DFMBlockDevice::~DFMBlockDevice()
{

}

bool DFMBlockDevice::eject()
{
    Q_D(DFMBlockDevice);

    return d->eject();
}

bool DFMBlockDevice::powerOff()
{
    Q_D(DFMBlockDevice);

    return d->powerOff();
}

DFMBlockDevicePrivate::DFMBlockDevicePrivate(const QString &dev)
    : devDesc(dev)
{

}

QString DFMBlockDevicePrivate::path() const
{
    return devDesc;
}

QUrl DFMBlockDevicePrivate::mount(const QVariantMap &opts)
{
    qDebug() << __PRETTY_FUNCTION__ << "is triggered";
    Q_Q(DFMBlockDevice);

    Q_EMIT q->mounted(QUrl());
    return QUrl();
}

void DFMBlockDevicePrivate::mountAsync(const QVariantMap &opts)
{
    Q_Q(DFMBlockDevice);

    QtConcurrent::run([&]{
        auto ret = mount(opts);
        Q_EMIT q->mounted(ret);
    });
}

bool DFMBlockDevicePrivate::unmount()
{
    return false;
}

void DFMBlockDevicePrivate::unmountAsync()
{
    Q_Q(DFMBlockDevice);

    QtConcurrent::run([&]{
        auto ret = unmount();
        if (ret)
            Q_EMIT q->unmounted();
    });
}

bool DFMBlockDevicePrivate::rename(const QString &newName)
{
    return false;
}

void DFMBlockDevicePrivate::renameAsync(const QString &newName)
{
    Q_Q(DFMBlockDevice);

    QtConcurrent::run([&]{
        auto ret = rename(newName);
        if (ret)
            Q_EMIT q->renamed(newName);
    });
}

QUrl DFMBlockDevicePrivate::accessPoint() const
{
    return QUrl();
}

QUrl DFMBlockDevicePrivate::mountPoint() const
{
    return QUrl();
}

QString DFMBlockDevicePrivate::fileSystem() const
{
    return QString();
}

long DFMBlockDevicePrivate::sizeTotal() const
{
    return 0;
}

long DFMBlockDevicePrivate::sizeUsage() const
{
    return 0;
}

long DFMBlockDevicePrivate::sizeFree() const
{
    return 0;
}

DeviceType DFMBlockDevicePrivate::deviceType() const
{
    return DeviceType::BlockDevice;
}

bool DFMBlockDevicePrivate::eject()
{
    return false;
}

bool DFMBlockDevicePrivate::powerOff()
{
    return false;
}
