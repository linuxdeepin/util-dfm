// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBLOCKDEVICE_P_H
#define DBLOCKDEVICE_P_H

#include <dfm-mount/base/dmount_global.h>
#include <dfm-mount/dblockdevice.h>

#include <libmount.h>

#include "private/ddevice_p.h"

extern "C" {
#include <udisks/udisks-generated.h>
}

DFM_MOUNT_BEGIN_NS

class DBlockDevicePrivate final : public DDevicePrivate
{
    friend class DBlockDevice;

public:
    DBlockDevicePrivate(UDisksClient *cli, const QString &blkObjPath, DBlockDevice *qq);
    ~DBlockDevicePrivate();

    QString path() const DMNT_OVERRIDE;
    QString mount(const QVariantMap &opts) DMNT_OVERRIDE;
    void mountAsync(const QVariantMap &opts, DeviceOperateCallbackWithMessage cb) DMNT_OVERRIDE;
    bool unmount(const QVariantMap &opts) DMNT_OVERRIDE;
    void unmountAsync(const QVariantMap &opts, DeviceOperateCallback cb) DMNT_OVERRIDE;
    bool rename(const QString &newName, const QVariantMap &opts) DMNT_OVERRIDE;
    void renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCallback cb) DMNT_OVERRIDE;
    QString mountPoint() const DMNT_OVERRIDE;
    QString fileSystem() const DMNT_OVERRIDE;
    qint64 sizeTotal() const DMNT_OVERRIDE;
    qint64 sizeUsage() const DMNT_OVERRIDE;
    qint64 sizeFree() const DMNT_OVERRIDE;
    DeviceType deviceType() const DMNT_OVERRIDE;
    QVariant getProperty(Property name) const DMNT_OVERRIDE;
    QString displayName() const DMNT_OVERRIDE;

    QVariant getBlockProperty(Property name) const;
    QVariant getDriveProperty(Property name) const;
    QVariant getFileSystemProperty(Property name) const;
    QVariant getPartitionProperty(Property name) const;
    QVariant getEncryptedProperty(Property name) const;

    bool eject(const QVariantMap &opts);
    void ejectAsync(const QVariantMap &opts, DeviceOperateCallback cb);
    bool powerOff(const QVariantMap &opts);
    void powerOffAsync(const QVariantMap &opts, DeviceOperateCallback cb);
    bool lock(const QVariantMap &opts);
    void lockAsync(const QVariantMap &opts, DeviceOperateCallback cb);
    bool unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts);
    void unlockAsync(const QString &passwd, const QVariantMap &opts, DeviceOperateCallbackWithMessage cb);
    bool rescan(const QVariantMap &opts);
    void rescanAsync(const QVariantMap &opts, DeviceOperateCallback cb);

private:
    // error report
    void handleErrorAndRelease(GError *err);
    static void handleErrorAndRelease(CallbackProxy *proxy, bool result, GError *gerr, QString info = QString());

    // async callbacks
    static void mountAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);
    static void unmountAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);
    static void renameAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);
    static void ejectAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);
    static void powerOffAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);
    static void lockAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);
    static void unlockAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);
    static void rescanAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData);

    static int dedupMountPoint(struct libmnt_table *table, struct libmnt_fs *a, struct libmnt_fs *b);
    static QString findFirstMountPoint(const QString &device);

    UDisksObject_autoptr getUDisksObject() const;
    UDisksBlock_autoptr getBlockHandler() const;
    UDisksDrive_autoptr getDriveHandler() const;
    UDisksLoop_autoptr getLoopHandler() const;
    UDisksEncrypted_autoptr getEncryptedHandler() const;
    UDisksPartition_autoptr getPartitionHandler() const;
    UDisksPartitionTable_autoptr getPartitionTableHandler() const;
    UDisksFilesystem_autoptr getFilesystemHandler() const;

    enum JobType {
        kBlockJob,
        kDriveJob
    };
    bool findJob(JobType type);

    QString blkObjPath;   // path of block device object
    UDisksClient *client { nullptr };
};
DFM_MOUNT_END_NS

#endif   // DBLOCKDEVICE_P_H
