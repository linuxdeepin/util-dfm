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

DFM_MOUNT_USE_NS
DFMBlockDevice::DFMBlockDevice(const QString &device, QObject *parent)
    :DFMAbstractDevice(parent), d_ptr(new DFMBlockDevicePrivate(this, device))
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
    q_ptr->iface.mount = std::bind(&DFMBlockDevicePrivate::mount, this, std::placeholders::_1);
    q_ptr->iface.mountAsync = std::bind(&DFMBlockDevicePrivate::mountAsync, this, std::placeholders::_1);
    q_ptr->iface.unmount = std::bind(&DFMBlockDevicePrivate::unmount, this);
    q_ptr->iface.unmountAsync = std::bind(&DFMBlockDevicePrivate::unmountAsync, this);
    q_ptr->iface.rename = std::bind(&DFMBlockDevicePrivate::rename, this, std::placeholders::_1);
    q_ptr->iface.renameAsync = std::bind(&DFMBlockDevicePrivate::renameAsync, this, std::placeholders::_1);
    q_ptr->iface.accessPoint = std::bind(&DFMBlockDevicePrivate::accessPoint, this);
    q_ptr->iface.mountPoint = std::bind(&DFMBlockDevicePrivate::mountPoint, this);
    q_ptr->iface.fileSystem = std::bind(&DFMBlockDevicePrivate::fileSystem, this);
    q_ptr->iface.sizeTotal = std::bind(&DFMBlockDevicePrivate::sizeTotal, this);
    q_ptr->iface.sizeUsage = std::bind(&DFMBlockDevicePrivate::sizeUsage, this);
    q_ptr->iface.sizeFree = std::bind(&DFMBlockDevicePrivate::sizeFree, this);
    q_ptr->iface.deviceType = std::bind(&DFMBlockDevicePrivate::deviceType, this);
}

QUrl DFMBlockDevicePrivate::mount(const QVariantMap &opts)
{
    qDebug() << __FUNCTION__ << "is triggered";
    Q_Q(DFMBlockDevice);
    emit q->mounted(QUrl());
    return QUrl();
}

void DFMBlockDevicePrivate::mountAsync(const QVariantMap &opts)
{
    Q_Q(DFMBlockDevice);
    QtConcurrent::run([&]{
        auto ret = mount(opts);
        emit q->mounted(ret);
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
            emit q->unmounted();
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
            emit q->renamed(newName);
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
