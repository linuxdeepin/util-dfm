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

#include "dfmprotocoldevice.h"
#include "private/dfmprotocoldevice_p.h"

#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include <functional>


DFM_MOUNT_USE_NS

DFMProtocolDevice::DFMProtocolDevice(const QString &device, QObject *parent)
    : DFMDevice(parent),
      d_pointer(new DFMProtocolDevicePrivate(this, device))
{

}

DFMProtocolDevice::~DFMProtocolDevice()
{

}

DFMProtocolDevicePrivate::DFMProtocolDevicePrivate(DFMProtocolDevice *qq, const QString &dev)
    : devDesc(dev),
      q_ptr(qq)
{
    qq->registerPath(std::bind(&DFMProtocolDevicePrivate::path, this));
    qq->registerMount(std::bind(&DFMProtocolDevicePrivate::mount, this, std::placeholders::_1));
    qq->registerMountAsync(std::bind(&DFMProtocolDevicePrivate::mountAsync, this, std::placeholders::_1));
    qq->registerUnmount(std::bind(&DFMProtocolDevicePrivate::unmount, this));
    qq->registerUnmountAsync(std::bind(&DFMProtocolDevicePrivate::unmountAsync, this));
    qq->registerRename(std::bind(&DFMProtocolDevicePrivate::rename, this, std::placeholders::_1));
    qq->registerRenameAsync(std::bind(&DFMProtocolDevicePrivate::renameAsync, this, std::placeholders::_1));
    qq->registerAccessPoint(std::bind(&DFMProtocolDevicePrivate::accessPoint, this));
    qq->registerMountPoint(std::bind(&DFMProtocolDevicePrivate::mountPoint, this));
    qq->registerFileSystem(std::bind(&DFMProtocolDevicePrivate::fileSystem, this));
    qq->registerSizeTotal(std::bind(&DFMProtocolDevicePrivate::sizeTotal, this));
    qq->registerSizeUsage(std::bind(&DFMProtocolDevicePrivate::sizeUsage, this));
    qq->registerSizeFree(std::bind(&DFMProtocolDevicePrivate::sizeFree, this));
    qq->registerDeviceType(std::bind(&DFMProtocolDevicePrivate::deviceType, this));
}

QString DFMProtocolDevicePrivate::path() const
{
    return devDesc;
}

QUrl DFMProtocolDevicePrivate::mount(const QVariantMap &opts)
{
    qDebug() << __PRETTY_FUNCTION__ << "is triggered";
    Q_Q(DFMProtocolDevice);
    Q_EMIT q->mounted(QUrl());
    return QUrl();
}

void DFMProtocolDevicePrivate::mountAsync(const QVariantMap &opts)
{
    Q_Q(DFMProtocolDevice);
    QtConcurrent::run([&]{
        auto ret = mount(opts);
        Q_EMIT q->mounted(ret);
    });
}

bool DFMProtocolDevicePrivate::unmount()
{
    return false;
}

void DFMProtocolDevicePrivate::unmountAsync()
{
    Q_Q(DFMProtocolDevice);
    QtConcurrent::run([&]{
        auto ret = unmount();
        if (ret)
            Q_EMIT q->unmounted();
    });
}

bool DFMProtocolDevicePrivate::rename(const QString &newName)
{
    return false;
}

void DFMProtocolDevicePrivate::renameAsync(const QString &newName)
{
    Q_Q(DFMProtocolDevice);
    QtConcurrent::run([&]{
        auto ret = rename(newName);
        if (ret)
            Q_EMIT q->renamed(newName);
    });
}

QUrl DFMProtocolDevicePrivate::accessPoint() const
{
    return QUrl();
}

QUrl DFMProtocolDevicePrivate::mountPoint() const
{
    return QUrl();
}

QString DFMProtocolDevicePrivate::fileSystem() const
{
    return QString();
}

long DFMProtocolDevicePrivate::sizeTotal() const
{
    return 0;
}

long DFMProtocolDevicePrivate::sizeUsage() const
{
    return 0;
}

long DFMProtocolDevicePrivate::sizeFree() const
{
    return 0;
}

DeviceType DFMProtocolDevicePrivate::deviceType() const
{
    return DeviceType::ProtocolDevice;
}

bool DFMProtocolDevicePrivate::eject()
{
    return false;
}

bool DFMProtocolDevicePrivate::powerOff()
{
    return false;
}
