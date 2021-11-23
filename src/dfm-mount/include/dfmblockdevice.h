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
#ifndef DFMBLOCKDEVICE_H
#define DFMBLOCKDEVICE_H

#include "base/dfmdevice.h"
#include "base/dfmmountdefines.h"

#include <QObject>

typedef struct _UDisksClient UDisksClient;

DFM_MOUNT_BEGIN_NS

class DFMBlockDevicePrivate;
class DFMBlockDevice final : public DFMDevice
{
    Q_OBJECT

public:
    DFMBlockDevice() = delete;
    DFMBlockDevice(const DFMBlockDevice &) = delete;
    ~DFMBlockDevice();

private:
    DFMBlockDevice(UDisksClient *cli, const QString &udisksObjPath, QObject *parent = nullptr);

public:
    bool eject(const QVariantMap &opts = {});
    void ejectAsync(const QVariantMap &opts = {}, DeviceOperateCb cb = nullptr);
    bool powerOff(const QVariantMap &opts = {});
    void powerOffAsync(const QVariantMap &opts = {}, DeviceOperateCb cb = nullptr);
    bool lock(const QVariantMap &opts = {});
    void lockAsync(const QVariantMap &opts = {}, DeviceOperateCb cb = nullptr);
    bool unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts = {});
    void unlockAsync(const QString &passwd, const QVariantMap &opts = {}, DeviceOperateCbWithInfo cb = nullptr);

    // these are convinience methods
    QStringList mountPoints() const;
    QString device() const;
    QString drive() const;
    QString idLabel() const;
    bool removable() const;
    bool optical() const;
    bool opticalBlank() const;
    QStringList mediaCompatibility() const;
    bool canPowerOff() const;
    bool ejectable() const;
    bool isEncrypted() const;
    bool hasFileSystem() const;
    bool hasPartitionTable() const;
    bool isLoopDevice() const;
    bool hintIgnore() const;
    bool hintSystem() const;
    PartitionType partitionEType() const;
    QString partitionType() const;

private:
    friend class DFMBlockMonitorPrivate;
};

DFM_MOUNT_END_NS

#endif // DFMBLOCKDEVICE_H
