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
#include "base/dfmmountdefines.h"
#include "base/dfmmountutils.h"

#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include <functional>


DFM_MOUNT_USE_NS

DFMProtocolDevice::DFMProtocolDevice(const QString &device, QObject *parent)
    : DFMDevice(new DFMProtocolDevicePrivate(device, this), parent)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    registerPath(std::bind(&DFMProtocolDevicePrivate::path, dp));
    registerMount(std::bind(&DFMProtocolDevicePrivate::mount, dp, std::placeholders::_1));
    registerMountAsync(std::bind(&DFMProtocolDevicePrivate::mountAsync, dp, std::placeholders::_1, std::placeholders::_2));
    registerUnmount(std::bind(&DFMProtocolDevicePrivate::unmount, dp));
    registerUnmountAsync(std::bind(&DFMProtocolDevicePrivate::unmountAsync, dp, std::placeholders::_1, std::placeholders::_2));
    registerRename(std::bind(&DFMProtocolDevicePrivate::rename, dp, std::placeholders::_1));
    registerRenameAsync(std::bind(&DFMProtocolDevicePrivate::renameAsync, dp, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    registerMountPoint(std::bind(&DFMProtocolDevicePrivate::mountPoint, dp));
    registerFileSystem(std::bind(&DFMProtocolDevicePrivate::fileSystem, dp));
    registerSizeTotal(std::bind(&DFMProtocolDevicePrivate::sizeTotal, dp));
    registerSizeUsage(std::bind(&DFMProtocolDevicePrivate::sizeUsage, dp));
    registerSizeFree(std::bind(&DFMProtocolDevicePrivate::sizeFree, dp));
    registerDeviceType(std::bind(&DFMProtocolDevicePrivate::deviceType, dp));
}

DFMProtocolDevice::~DFMProtocolDevice()
{

}

DFMProtocolDevicePrivate::DFMProtocolDevicePrivate(const QString &dev, DFMProtocolDevice *qq)
    : DFMDevicePrivate(qq), devDesc(dev)
{

}

QString DFMProtocolDevicePrivate::path() const
{
    // TODO
    return devDesc;
}

QString DFMProtocolDevicePrivate::mount(const QVariantMap &opts)
{
    // TODO
    qDebug() << __PRETTY_FUNCTION__ << "is triggered";
    return "";
}

void DFMProtocolDevicePrivate::mountAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    // TODO
    QtConcurrent::run([&]{
        auto ret = mount(opts);
    });
}

bool DFMProtocolDevicePrivate::unmount()
{
    // TODO
    return false;
}

void DFMProtocolDevicePrivate::unmountAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    // TODO
    QtConcurrent::run([&]{
        auto ret = unmount();
        if (ret);
//            Q_EMIT q->unmounted();
    });
}

bool DFMProtocolDevicePrivate::rename(const QString &newName)
{
    return false;
}

void DFMProtocolDevicePrivate::renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCb cb)
{
    // TODO
    QtConcurrent::run([&]{
        auto ret = rename(newName);
        if (ret);
//            Q_EMIT q->renamed(newName);
    });
}

QString DFMProtocolDevicePrivate::mountPoint() const
{
    // TODO
    return QString();
}

QString DFMProtocolDevicePrivate::fileSystem() const
{
    // TODO
    return QString();
}

long DFMProtocolDevicePrivate::sizeTotal() const
{
    // TODO
    return 0;
}

long DFMProtocolDevicePrivate::sizeUsage() const
{
    // TODO
    return 0;
}

long DFMProtocolDevicePrivate::sizeFree() const
{
    // TODO
    return 0;
}

DeviceType DFMProtocolDevicePrivate::deviceType() const
{
    // TODO
    return DeviceType::ProtocolDevice;
}

bool DFMProtocolDevicePrivate::eject()
{
    // TODO
    return false;
}

bool DFMProtocolDevicePrivate::powerOff()
{
    // TODO
    return false;
}
