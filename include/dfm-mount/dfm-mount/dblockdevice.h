// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBLOCKDEVICE_H
#define DBLOCKDEVICE_H

#include <dfm-mount/base/ddevice.h>
#include <dfm-mount/base/dmount_global.h>

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
