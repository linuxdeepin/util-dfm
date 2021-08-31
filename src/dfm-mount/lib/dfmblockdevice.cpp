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
    :DFMDevice(parent), d_ptr(new DFMBlockDevicePrivate(this, device))
{
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

DFMBlockDevicePrivate::DFMBlockDevicePrivate(DFMBlockDevice *qq, const QString &dev)
    : devDesc(dev), q_ptr(qq)
{
    qq->registerPath(std::bind(&DFMBlockDevicePrivate::path, this));
    qq->registerMount(std::bind(&DFMBlockDevicePrivate::mount, this, std::placeholders::_1));
    qq->registerMountAsync(std::bind(&DFMBlockDevicePrivate::mountAsync, this, std::placeholders::_1));
    qq->registerUnmount(std::bind(&DFMBlockDevicePrivate::unmount, this));
    qq->registerUnmountAsync(std::bind(&DFMBlockDevicePrivate::unmountAsync, this));
    qq->registerRename(std::bind(&DFMBlockDevicePrivate::rename, this, std::placeholders::_1));
    qq->registerRenameAsync(std::bind(&DFMBlockDevicePrivate::renameAsync, this, std::placeholders::_1));
    qq->registerAccessPoint(std::bind(&DFMBlockDevicePrivate::accessPoint, this));
    qq->registerMountPoint(std::bind(&DFMBlockDevicePrivate::mountPoint, this));
    qq->registerFileSystem(std::bind(&DFMBlockDevicePrivate::fileSystem, this));
    qq->registerSizeTotal(std::bind(&DFMBlockDevicePrivate::sizeTotal, this));
    qq->registerSizeUsage(std::bind(&DFMBlockDevicePrivate::sizeUsage, this));
    qq->registerSizeFree(std::bind(&DFMBlockDevicePrivate::sizeFree, this));
    qq->registerDeviceType(std::bind(&DFMBlockDevicePrivate::deviceType, this));
}

QString DFMBlockDevicePrivate::path()
{
    return devDesc;
}

QUrl DFMBlockDevicePrivate::mount(const QVariantMap &opts)
{
    qDebug() << __FUNCTION__ << "is triggered";
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

QUrl DFMBlockDevicePrivate::accessPoint()
{
    return QUrl();
}

QUrl DFMBlockDevicePrivate::mountPoint()
{
    return QUrl();
}

QString DFMBlockDevicePrivate::fileSystem()
{
    return QString();
}

long DFMBlockDevicePrivate::sizeTotal()
{
    return 0;
}

long DFMBlockDevicePrivate::sizeUsage()
{
    return 0;
}

long DFMBlockDevicePrivate::sizeFree()
{
    return 0;
}

int DFMBlockDevicePrivate::deviceType()
{
    return BlockDevice;
}

bool DFMBlockDevicePrivate::eject()
{
    return false;
}

bool DFMBlockDevicePrivate::powerOff()
{
    return false;
}
