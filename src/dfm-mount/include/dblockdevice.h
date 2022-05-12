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
#ifndef DBLOCKDEVICE_H
#define DBLOCKDEVICE_H

#include "base/ddevice.h"
#include "base/dmount_global.h"

#include <QObject>

typedef struct _UDisksClient UDisksClient;

DFM_MOUNT_BEGIN_NS

class DBlockDevicePrivate;
class DBlockDevice final : public DDevice
{
    Q_OBJECT
    friend class DBlockMonitorPrivate;

public:
    DBlockDevice() = delete;
    DBlockDevice(const DBlockDevice &) = delete;
    ~DBlockDevice();

private:
    DBlockDevice(UDisksClient *cli, const QString &udisksObjPath, QObject *parent = nullptr);

public:
    bool eject(const QVariantMap &opts = {});
    void ejectAsync(const QVariantMap &opts = {}, DeviceOperateCallback cb = nullptr);
    bool powerOff(const QVariantMap &opts = {});
    void powerOffAsync(const QVariantMap &opts = {}, DeviceOperateCallback cb = nullptr);
    bool lock(const QVariantMap &opts = {});
    void lockAsync(const QVariantMap &opts = {}, DeviceOperateCallback cb = nullptr);
    bool unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts = {});
    void unlockAsync(const QString &passwd, const QVariantMap &opts = {}, DeviceOperateCallbackWithMessage cb = nullptr);
    bool rescan(const QVariantMap &opts = {});
    void rescanAsync(const QVariantMap &opts = {}, DeviceOperateCallback cb = nullptr);

    // these are convinience methods
    QStringList mountPoints() const;
    QString device() const;
    QString drive() const;
    QString idLabel() const;
    QStringList mediaCompatibility() const;
    bool removable() const;
    bool optical() const;
    bool opticalBlank() const;
    bool canPowerOff() const;
    bool ejectable() const;

    bool isEncrypted() const;
    bool hasFileSystem() const;
    bool hasPartitionTable() const;
    bool hasPartition() const;
    bool isLoopDevice() const;
    bool hasBlock() const;

    bool hintIgnore() const;
    bool hintSystem() const;

    PartitionType partitionEType() const;
    QString partitionType() const;
};

DFM_MOUNT_END_NS

#endif   // DBLOCKDEVICE_H
