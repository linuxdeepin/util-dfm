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

#include "dfmmountdefines.h"
#include "dfmabstractdevice.h"

#include <QDebug>

DFM_MOUNT_USE_NS

DFMAbstractDevice::DFMAbstractDevice(QObject *parent)
    :QObject (parent)
{

}

QString DFMAbstractDevice::path()
{
    if (iface.path)
        return iface.path();
    WARN_NO_INIT();
    return QString();
}

QUrl DFMAbstractDevice::mount(const QVariantMap &opts)
{
    if (iface.mount)
        return iface.mount(opts);
    WARN_NO_INIT();
    return QUrl();
}

void DFMAbstractDevice::mountAsync(const QVariantMap &opts)
{
    if (iface.mountAsync)
        return iface.mountAsync(opts);
    WARN_NO_INIT();
}

bool DFMAbstractDevice::unmount()
{
    if (iface.unmount)
        return iface.unmount();
    WARN_NO_INIT();
    return false;
}

void DFMAbstractDevice::unmountAsync()
{
    if (iface.unmountAsync)
        return iface.unmountAsync();
    WARN_NO_INIT();
}

bool DFMAbstractDevice::rename(const QString &newName)
{
    if (iface.rename)
        return iface.rename(newName);
    WARN_NO_INIT();
    return false;
}

void DFMAbstractDevice::renameAsync(const QString &newName)
{
    if (iface.renameAsync)
        return iface.renameAsync(newName);
    WARN_NO_INIT();
}

QUrl DFMAbstractDevice::accessPoint()
{
    if (iface.accessPoint)
        return iface.accessPoint();
    WARN_NO_INIT();
    return QUrl();
}

QUrl DFMAbstractDevice::mountPoint()
{
    if (iface.mountPoint)
        return iface.mountPoint();
    WARN_NO_INIT();
    return QUrl();
}

QString DFMAbstractDevice::fileSystem()
{
    if (iface.fileSystem)
        return iface.fileSystem();
    WARN_NO_INIT();
    return QString();
}

long DFMAbstractDevice::sizeTotal()
{
    if (iface.sizeTotal)
        return iface.sizeTotal();
    WARN_NO_INIT();
    return 0;
}

long DFMAbstractDevice::sizeFree()
{
    if (iface.sizeFree)
        return iface.sizeFree();
    WARN_NO_INIT();
    return 0;
}

long DFMAbstractDevice::sizeUsage()
{
    if (iface.sizeUsage)
        return iface.sizeUsage();
    WARN_NO_INIT();
    return 0;
}

int DFMAbstractDevice::deviceType()
{
    if (iface.deviceType)
        return iface.deviceType();
    WARN_NO_INIT();
    return 0;
}

QVariant DFMAbstractDevice::getProperty(Property item)
{
    if (iface.getProperty)
        return iface.getProperty(item);
    WARN_NO_INIT();
    return QVariant();
}
