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

#include "base/dmount_global.h"
#include "base/dmountutils.h"
#include "dblockdevice.h"
#include "private/dblockdevice_p.h"

#include <QStorageInfo>
#include <QDebug>

#include <functional>

extern "C" {
#include <udisks/udisks.h>
}

DFM_MOUNT_USE_NS

inline void DBlockDevicePrivate::handleErrorAndRelease(CallbackProxy *proxy, bool result, GError *gerr, QString info)
{
    DeviceError err = DeviceError::kNoError;
    if (!result) {
        err = Utils::castFromGError(gerr);
        g_error_free(gerr);
    }

    if (proxy) {
        if (proxy->cb) {
            proxy->cb(result, err);
        } else if (proxy->cbWithInfo) {
            proxy->cbWithInfo(result, err, info);
        }
        delete proxy;
    }
}

void DBlockDevicePrivate::mountAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(sourceObj);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    g_autofree char *mountPoint = nullptr;
    bool result = udisks_filesystem_call_mount_finish(fs, &mountPoint, res, &err);
    handleErrorAndRelease(proxy, result, err, mountPoint);   // ignore mount point, which will be notified by onPropertyChanged
};

void DBlockDevicePrivate::unmountAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(sourceObj);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_filesystem_call_unmount_finish(fs, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DBlockDevicePrivate::renameAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(sourceObj);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_filesystem_call_set_label_finish(fs, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DBlockDevicePrivate::ejectAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksDrive *drive = UDISKS_DRIVE(sourceObj);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_drive_call_eject_finish(drive, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DBlockDevicePrivate::powerOffAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksDrive *drive = UDISKS_DRIVE(sourceObj);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_drive_call_power_off_finish(drive, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DBlockDevicePrivate::lockAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksEncrypted *encrypted = UDISKS_ENCRYPTED(sourceObj);
    Q_ASSERT_X(encrypted, __FUNCTION__, "encrypted is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_encrypted_call_lock_finish(encrypted, res, &err);
    handleErrorAndRelease(proxy, result, err);
}

void DBlockDevicePrivate::unlockAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksEncrypted *encrypted = UDISKS_ENCRYPTED(sourceObj);
    Q_ASSERT_X(encrypted, __FUNCTION__, "encrypted is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    g_autofree char *clearTextDev = nullptr;
    bool result = udisks_encrypted_call_unlock_finish(encrypted, &clearTextDev, res, &err);
    handleErrorAndRelease(proxy, result, err, QString(clearTextDev));
}

void DBlockDevicePrivate::rescanAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksBlock *block = UDISKS_BLOCK(sourceObj);
    Q_ASSERT_X(block, __FUNCTION__, "block is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_block_call_rescan_finish(block, res, &err);
    handleErrorAndRelease(proxy, result, err);
}

UDisksObject_autoptr DBlockDevicePrivate::getUDisksObject() const
{
    Q_ASSERT(client);
    Q_ASSERT(!blkObjPath.isEmpty());

    std::string str = blkObjPath.toStdString();
    UDisksObject_autoptr obj = udisks_client_get_object(client, str.c_str());
    return obj;
}

UDisksBlock_autoptr DBlockDevicePrivate::getBlockHandler() const
{
    UDisksObject_autoptr obj = getUDisksObject();
    if (!obj) {
        qWarning() << "UDisksObject is not valid for" << blkObjPath;
        return nullptr;
    }

    UDisksBlock_autoptr ret = udisks_object_get_block(obj);
    return ret;
}

UDisksDrive_autoptr DBlockDevicePrivate::getDriveHandler() const
{
    Q_ASSERT(client);

    UDisksBlock_autoptr blk = getBlockHandler();
    if (!blk) {
        qWarning() << "UDisksBlock is not valid for" << blkObjPath;
        return nullptr;
    }

    UDisksDrive_autoptr ret = udisks_client_get_drive_for_block(client, blk);
    return ret;
}

UDisksLoop_autoptr DBlockDevicePrivate::getLoopHandler() const
{
    Q_ASSERT(client);

    UDisksBlock_autoptr blk = getBlockHandler();
    if (!blk) {
        qWarning() << "UDisksBlock is not valid for" << blkObjPath;
        return nullptr;
    }

    UDisksLoop_autoptr ret = udisks_client_get_loop_for_block(client, blk);
    return ret;
}

UDisksEncrypted_autoptr DBlockDevicePrivate::getEncryptedHandler() const
{
    UDisksObject_autoptr obj = getUDisksObject();
    if (!obj) {
        qWarning() << "UDisksObject is not valid for" << blkObjPath;
        return nullptr;
    }

    UDisksEncrypted_autoptr ret = udisks_object_get_encrypted(obj);
    return ret;
}

UDisksPartition_autoptr DBlockDevicePrivate::getPartitionHandler() const
{
    UDisksObject_autoptr obj = getUDisksObject();
    if (!obj) {
        qWarning() << "UDisksObject is not valid for" << blkObjPath;
        return nullptr;
    }

    UDisksPartition_autoptr ret = udisks_object_get_partition(obj);
    return ret;
}

UDisksPartitionTable_autoptr DBlockDevicePrivate::getPartitionTableHandler() const
{
    UDisksObject_autoptr obj = getUDisksObject();
    if (!obj) {
        qWarning() << "UDisksObject is not valid for" << blkObjPath;
        return nullptr;
    }

    UDisksPartitionTable_autoptr ret = udisks_object_get_partition_table(obj);
    return ret;
}

UDisksFilesystem_autoptr DBlockDevicePrivate::getFilesystemHandler() const
{
    UDisksObject_autoptr obj = getUDisksObject();
    if (!obj) {
        qWarning() << "UDisksObject is not valid for" << blkObjPath;
        return nullptr;
    }

    UDisksFilesystem_autoptr ret = udisks_object_get_filesystem(obj);
    return ret;
}

bool DBlockDevicePrivate::findJob(JobType type)
{
    QString objPath = blkObjPath;
    if (type == kDriveJob)
        objPath = getBlockProperty(Property::kBlockDrive).toString();

    if (objPath == "/")   // loop devices' drive field is '/'
        return false;

    UDisksObject_autoptr obj = udisks_client_get_object(client, objPath.toStdString().c_str());
    if (!obj)
        return false;

    struct UserData
    {
        DBlockDevicePrivate *d { nullptr };
        QString objPath;
        bool hasJob = false;
    };

    UserData data { this, blkObjPath, false };
    GList_autoptr jobs = udisks_client_get_jobs_for_object(client, obj);
    g_list_foreach(jobs, [](void *item, void *that) {
        UDisksJob *job = static_cast<UDisksJob *>(item);
        UserData *d = static_cast<UserData *>(that);
        if (!job || !d)
            return;
        QString op(udisks_job_get_operation(job));
        qInfo() << "Working now..." << d->objPath << op;
        d->hasJob = true;
        d->d->lastError = Utils::castFromJobOperation(op);
    },
                   &data);

    return data.hasJob;
}

DBlockDevice::DBlockDevice(UDisksClient *cli, const QString &udisksObjPath, QObject *parent)
    : DDevice(new DBlockDevicePrivate(cli, udisksObjPath, this), parent)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }

    using namespace std;
    using namespace std::placeholders;
    registerPath(bind(&DBlockDevicePrivate::path, dp));
    registerMount(bind(&DBlockDevicePrivate::mount, dp, _1));
    registerMountAsync(bind(&DBlockDevicePrivate::mountAsync, dp, _1, _2));
    registerUnmount(bind(&DBlockDevicePrivate::unmount, dp, _1));
    registerUnmountAsync(bind(&DBlockDevicePrivate::unmountAsync, dp, _1, _2));
    registerRename(bind(&DBlockDevicePrivate::rename, dp, _1, _2));
    registerRenameAsync(bind(&DBlockDevicePrivate::renameAsync, dp, _1, _2, _3));
    registerMountPoint(bind(&DBlockDevicePrivate::mountPoint, dp));
    registerFileSystem(bind(&DBlockDevicePrivate::fileSystem, dp));
    registerSizeTotal(bind(&DBlockDevicePrivate::sizeTotal, dp));
    registerSizeUsage(bind(&DBlockDevicePrivate::sizeUsage, dp));
    registerSizeFree(bind(&DBlockDevicePrivate::sizeFree, dp));
    registerDeviceType(bind(&DBlockDevicePrivate::deviceType, dp));
    registerGetProperty(bind(&DBlockDevicePrivate::getProperty, dp, _1));
    registerDisplayName(bind(&DBlockDevicePrivate::displayName, dp));
}

DBlockDevice::~DBlockDevice()
{
}

bool DBlockDevice::eject(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->eject(opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DBlockDevice::ejectAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->ejectAsync(opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

bool DBlockDevice::powerOff(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->powerOff(opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DBlockDevice::powerOffAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        dp->powerOffAsync(opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

bool DBlockDevice::lock(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->lock(opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DBlockDevice::lockAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        dp->lockAsync(opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

bool DBlockDevice::unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->unlock(passwd, clearTextDev, opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DBlockDevice::unlockAsync(const QString &passwd, const QVariantMap &opts, DeviceOperateCallbackWithMessage cb)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        dp->unlockAsync(passwd, opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

bool DBlockDevice::rescan(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp)
        return dp->rescan(opts);
    else
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    return false;
}

void DBlockDevice::rescanAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    if (dp) {
        dp->rescanAsync(opts, cb);
    } else {
        if (cb)
            cb(false, DeviceError::kUserErrorFailed);
        qWarning() << "private pointer is null!";
    }
}

QStringList DBlockDevice::mountPoints() const
{
    return getProperty(Property::kFileSystemMountPoint).toStringList();
}

QString DBlockDevice::device() const
{
    return getProperty(Property::kBlockDevice).toString();
}

QString DBlockDevice::drive() const
{
    return getProperty(Property::kBlockDrive).toString();
}

QString DBlockDevice::idLabel() const
{
    return getProperty(Property::kBlockIDLabel).toString();
}

bool DBlockDevice::removable() const
{
    return getProperty(Property::kDriveRemovable).toBool();
}

bool DBlockDevice::optical() const
{
    return getProperty(Property::kDriveOptical).toBool();
}

bool DBlockDevice::opticalBlank() const
{
    return getProperty(Property::kDriveOpticalBlank).toBool();
}

QStringList DBlockDevice::mediaCompatibility() const
{
    return getProperty(Property::kDriveMediaCompatibility).toStringList();
}

bool DBlockDevice::canPowerOff() const
{
    return getProperty(Property::kDriveCanPowerOff).toBool();
}

bool DBlockDevice::ejectable() const
{
    return getProperty(Property::kDriveEjectable).toBool();
}

bool DBlockDevice::isEncrypted() const
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    return dp ? dp->getEncryptedHandler() != nullptr : false;
}

bool DBlockDevice::hasFileSystem() const
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    return dp ? dp->getFilesystemHandler() != nullptr : false;
}

bool DBlockDevice::hasPartitionTable() const
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    return dp ? dp->getPartitionTableHandler() != nullptr : false;
}

bool DBlockDevice::hasPartition() const
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    return dp ? dp->getPartitionHandler() != nullptr : false;
}

bool DBlockDevice::isLoopDevice() const
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    return dp ? dp->getLoopHandler() != nullptr : false;
}

bool DBlockDevice::hintIgnore() const
{
    return getProperty(Property::kBlockHintIgnore).toBool();
}

bool DBlockDevice::hintSystem() const
{
    return getProperty(Property::kBlockHintSystem).toBool();
}

PartitionType DBlockDevice::partitionEType() const
{
    auto typestr = partitionType();
    if (typestr.isEmpty())
        return PartitionType::kPartitionTypeNotFound;
    bool ok = false;
    int type = typestr.toInt(&ok, 16);
    if (ok) {
        if (type >= static_cast<int>(PartitionType::kMbrEmpty)
            && type <= static_cast<int>(PartitionType::kMbrBBT))
            return static_cast<PartitionType>(type);
        else
            return PartitionType::kPartitionTypeNotFound;
    } else {
        return Utils::getPartitionTypeByGuid(typestr);
    }
}

QString DBlockDevice::partitionType() const
{
    return getProperty(Property::kPartitionType).toString();
}

bool DBlockDevice::hasBlock() const
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DBlockDevicePrivate>(d.data());
    return dp ? dp->getBlockHandler() != nullptr : false;
}

DBlockDevicePrivate::DBlockDevicePrivate(UDisksClient *cli, const QString &blkObjPath, DBlockDevice *qq)
    : DDevicePrivate(qq), blkObjPath(blkObjPath), client(cli)
{
}

DBlockDevicePrivate::~DBlockDevicePrivate()
{
}

QString DBlockDevicePrivate::path() const
{
    return blkObjPath;
}

QString DBlockDevicePrivate::mount(const QVariantMap &opts)
{
    warningIfNotInMain();

    if (findJob(kBlockJob))
        return "";

    UDisksFilesystem_autoptr fs = getFilesystemHandler();
    if (!fs) {
        lastError = DeviceError::kUserErrorNotMountable;
        return "";
    }

    QStringList mpts = getProperty(Property::kFileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::kUDisksErrorAlreadyMounted;
        return mpts.first();
    }

    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    char *mountPoint = nullptr;
    bool mounted = udisks_filesystem_call_mount_sync(fs, gopts, &mountPoint, nullptr, &err);

    handleErrorAndRelease(err);
    QString ret;
    if (mounted && mountPoint) {
        ret = mountPoint;
        g_free(mountPoint);
    }
    return ret;
}

void DBlockDevicePrivate::mountAsync(const QVariantMap &opts, DeviceOperateCallbackWithMessage cb)
{
    if (findJob(kBlockJob)) {
        if (cb)
            cb(false, lastError, "");
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    UDisksFilesystem_autoptr fs = getFilesystemHandler();

    if (!fs) {
        lastError = DeviceError::kUserErrorNotMountable;
        if (proxy) {
            proxy->cbWithInfo(false, lastError, "");
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::kFileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::kUDisksErrorAlreadyMounted;
        if (proxy) {
            proxy->cbWithInfo(true, lastError, mpts.first());   // when it's already mounted, return true but report an error refer to `already mounted`.
            delete proxy;
        }
        return;
    }

    // mount device async
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_filesystem_call_mount(fs, gopts, nullptr, mountAsyncCallback, proxy);
}

bool DBlockDevicePrivate::unmount(const QVariantMap &opts)
{
    warningIfNotInMain();
    if (findJob(kBlockJob))
        return false;

    UDisksFilesystem_autoptr fs = getFilesystemHandler();

    if (!fs) {
        lastError = DeviceError::kUserErrorNotMountable;
        return true;   // since device is not mountable, so we just return true here
    }

    QStringList mpts = getProperty(Property::kFileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = DeviceError::kUDisksErrorNotMounted;
        return true;   // since it's not mounted, then this invocation returns true
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;
    bool result = udisks_filesystem_call_unmount_sync(fs, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelease(err);
    return false;
}

void DBlockDevicePrivate::unmountAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (findJob(kBlockJob)) {
        if (cb)
            cb(false, lastError);
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    UDisksFilesystem_autoptr fs = getFilesystemHandler();

    if (!fs) {
        lastError = DeviceError::kUserErrorNotMountable;
        if (proxy) {
            proxy->cb(true, lastError);
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::kFileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = DeviceError::kUDisksErrorNotMounted;
        if (proxy) {
            proxy->cb(true, lastError);
            delete proxy;
        }
        return;
    }

    // start unmount device async
    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_filesystem_call_unmount(fs, gopts, nullptr, unmountAsyncCallback, proxy);
}

bool DBlockDevicePrivate::rename(const QString &newName, const QVariantMap &opts)
{
    warningIfNotInMain();
    if (findJob(kBlockJob))
        return false;

    UDisksFilesystem_autoptr fs = getFilesystemHandler();

    if (!fs) {
        lastError = DeviceError::kUserErrorNotMountable;
        return false;
    }

    QStringList mpts = getProperty(Property::kFileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::kUDisksErrorAlreadyMounted;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    std::string name = newName.toStdString();
    const char *label = name.data();
    GError *err = nullptr;

    bool result = udisks_filesystem_call_set_label_sync(fs, label, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelease(err);
    return false;
}

void DBlockDevicePrivate::renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (findJob(kBlockJob)) {
        if (cb)
            cb(false, lastError);
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    UDisksFilesystem_autoptr fs = getFilesystemHandler();

    if (!fs) {
        lastError = DeviceError::kUserErrorNotMountable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::kFileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::kUDisksErrorAlreadyMounted;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    std::string name = newName.toStdString();
    const char *label = name.data();
    udisks_filesystem_call_set_label(fs, label, gopts, nullptr, renameAsyncCallback, proxy);
}

bool DBlockDevicePrivate::eject(const QVariantMap &opts)
{
    warningIfNotInMain();
    if (findJob(kBlockJob))
        return false;

    UDisksDrive_autoptr drv = getDriveHandler();
    if (!drv) {
        lastError = DeviceError::kUserErrorNoDriver;
        return false;
    }

    bool ejectable = q->getProperty(Property::kDriveEjectable).toBool();
    if (!ejectable) {
        lastError = DeviceError::kUserErrorNotEjectable;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;

    bool result = udisks_drive_call_eject_sync(drv, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelease(err);
    return false;
}

void DBlockDevicePrivate::ejectAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (findJob(kBlockJob)) {
        if (cb)
            cb(false, lastError);
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    bool ejectable = q->getProperty(Property::kDriveEjectable).toBool();
    if (!ejectable) {
        lastError = DeviceError::kUserErrorNotEjectable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    UDisksDrive_autoptr drv = getDriveHandler();

    if (!drv) {
        lastError = DeviceError::kUserErrorNoDriver;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_drive_call_eject(drv, gopts, nullptr, ejectAsyncCallback, proxy);
}

bool DBlockDevicePrivate::powerOff(const QVariantMap &opts)
{
    warningIfNotInMain();
    if (findJob(kBlockJob) || findJob(kDriveJob))
        return false;

    UDisksDrive_autoptr drv = getDriveHandler();

    if (!drv) {
        lastError = DeviceError::kUserErrorNoDriver;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;
    bool result = udisks_drive_call_power_off_sync(drv, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelease(err);
    return false;
}

void DBlockDevicePrivate::powerOffAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (findJob(kBlockJob) || findJob(kDriveJob)) {
        if (cb)
            cb(false, lastError);
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    UDisksDrive_autoptr drv = getDriveHandler();

    if (!drv) {
        lastError = DeviceError::kUserErrorNoDriver;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_drive_call_power_off(drv, gopts, nullptr, powerOffAsyncCallback, proxy);
}

bool DBlockDevicePrivate::lock(const QVariantMap &opts)
{
    warningIfNotInMain();
    if (findJob(kBlockJob))
        return false;

    UDisksEncrypted_autoptr encrypted = getEncryptedHandler();

    if (!encrypted) {
        lastError = DeviceError::kUserErrorNotEncryptable;
        return false;
    }

    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    bool result = udisks_encrypted_call_lock_sync(encrypted, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelease(err);
    return false;
}

void DBlockDevicePrivate::lockAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (findJob(kBlockJob)) {
        if (cb)
            cb(false, lastError);
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    UDisksEncrypted_autoptr encrypted = getEncryptedHandler();

    if (!encrypted) {
        lastError = DeviceError::kUserErrorNotEncryptable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_encrypted_call_lock(encrypted, gopts, nullptr, lockAsyncCallback, proxy);
}

bool DBlockDevicePrivate::unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts)
{
    warningIfNotInMain();
    if (findJob(kBlockJob))
        return false;

    UDisksEncrypted_autoptr encrypted = getEncryptedHandler();

    if (!encrypted) {
        lastError = DeviceError::kUserErrorNotEncryptable;
        return false;
    }

    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    char *clearDev = nullptr;
    bool result = udisks_encrypted_call_unlock_sync(encrypted, passwd.toStdString().c_str(), gopts, &clearDev, nullptr, &err);
    if (result) {
        clearTextDev = QString(clearDev);
        g_free(clearDev);
        return true;
    }

    handleErrorAndRelease(err);
    return false;
}

void DBlockDevicePrivate::unlockAsync(const QString &passwd, const QVariantMap &opts, DeviceOperateCallbackWithMessage cb)
{
    if (findJob(kBlockJob)) {
        if (cb)
            cb(false, lastError, "");
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    UDisksEncrypted_autoptr encrypted = getEncryptedHandler();

    if (!encrypted) {
        lastError = DeviceError::kUserErrorNotEncryptable;
        if (proxy) {
            proxy->cbWithInfo(false, lastError, QString());
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_encrypted_call_unlock(encrypted, passwd.toStdString().c_str(), gopts, nullptr, unlockAsyncCallback, proxy);
}

bool DBlockDevicePrivate::rescan(const QVariantMap &opts)
{
    warningIfNotInMain();

    if (findJob(kBlockJob)) return false;

    UDisksBlock_autoptr blk = getBlockHandler();
    GError_autoptr err = nullptr;
    if (blk) {
        bool ret = udisks_block_call_rescan_sync(blk, Utils::castFromQVariantMap(opts), nullptr, &err);
        if (err) {
            qWarning() << "error while rescaning: " << err->message;
            return false;
        }
        return ret;
    }
    return false;
}

void DBlockDevicePrivate::rescanAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (findJob(kBlockJob)) {
        if (cb)
            cb(false, lastError);
        return;
    }

    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    UDisksBlock_autoptr blk = getBlockHandler();

    if (blk) {
        udisks_block_call_rescan(blk, Utils::castFromQVariantMap(opts), nullptr, rescanAsyncCallback, proxy);
    } else {
        if (cb)
            cb(false, DeviceError::kUserErrorNoBlock);
        qWarning() << "cannot get block handler";
    }
}

inline void DBlockDevicePrivate::handleErrorAndRelease(GError *err)
{
    if (err) {
        lastError = Utils::castFromGError(err);
        g_error_free(err);
    }
}

QString DBlockDevicePrivate::mountPoint() const
{
    auto mpts = q->getProperty(Property::kFileSystemMountPoint).toStringList();
    return mpts.isEmpty() ? QString() : mpts.first();
}

QString DBlockDevicePrivate::fileSystem() const
{
    return getProperty(Property::kBlockIDType).toString();
}

qint64 DBlockDevicePrivate::sizeTotal() const
{
    return q->getProperty(Property::kBlockSize).toLongLong();
}

qint64 DBlockDevicePrivate::sizeUsage() const
{
    return sizeTotal() - sizeFree();
}

qint64 DBlockDevicePrivate::sizeFree() const
{
    auto mpts = q->getProperty(Property::kFileSystemMountPoint).toStringList();
    if (mpts.isEmpty()) {
        //        lastError = MountError::NotMounted;
        qInfo() << __FUNCTION__ << "NOT MOUNTED: " << blkObjPath;
        return 0;
    }
    auto mpt = mpts.first();
    QStorageInfo info(mpt);
    return info.bytesAvailable();
}

DeviceType DBlockDevicePrivate::deviceType() const
{
    return DeviceType::kBlockDevice;
}

QVariant DBlockDevicePrivate::getProperty(Property name) const
{
    if (name > Property::kBlockProperty && name < Property::kBlockPropertyEND)
        return getBlockProperty(name);
    else if (name > Property::kDriveProperty && name < Property::kDrivePropertyEND)
        return getDriveProperty(name);
    else if (name > Property::kFileSystemProperty && name < Property::kFileSystemPropertyEND)
        return getFileSystemProperty(name);
    else if (name > Property::kPartitionProperty && name < Property::kPartitionPropertyEND)
        return getPartitionProperty(name);
    else if (name > Property::kEncryptedProperty && name < Property::kEncryptedPropertyEnd)
        return getEncryptedProperty(name);

    //    Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
    return QVariant();
}

QString DBlockDevicePrivate::displayName() const
{
    return getProperty(Property::kBlockIDLabel).toString();
}

QVariant DBlockDevicePrivate::getBlockProperty(Property name) const
{
    UDisksBlock_autoptr blk = getBlockHandler();
    if (!blk) {
        qWarning() << __FUNCTION__ << "NO BLOCK: " << blkObjPath;
        lastError = DeviceError::kUserErrorNoBlock;
        return QVariant();
    }

    // make sure we can safely get the properties in cross-thread cases: so we use DUP rather than GET when DUP can be used.
    // but we shall release the objects by calling g_free for char * or g_strfreev for char ** funcs.
    switch (name) {
    case Property::kBlockConfiguration:
        //                return udisks_block_dup_configuration(blk); TODO
        return "";
    case Property::kBlockUserspaceMountOptions: {
        char **opts = udisks_block_dup_userspace_mount_options(blk);
        return Utils::gcharvToQStringList(opts);
    }
    case Property::kBlockCryptoBackingDevice: {
        char *tmp = udisks_block_dup_crypto_backing_device(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockDevice: {
        char *tmp = udisks_block_dup_device(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockDrive: {
        char *tmp = udisks_block_dup_drive(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockIDLabel: {
        char *tmp = udisks_block_dup_id_label(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockIDType: {
        char *tmp = udisks_block_dup_id_type(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockIDUsage: {
        char *tmp = udisks_block_dup_id_usage(blk);
        QString ret = Utils::gcharToQString(tmp);
        return ret.toLongLong();
    }
    case Property::kBlockIDUUID: {
        char *tmp = udisks_block_dup_id_uuid(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockIDVersion: {
        char *tmp = udisks_block_dup_id_version(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockDeviceNumber:
        return quint64(udisks_block_get_device_number(blk));
    case Property::kBlockPreferredDevice: {
        char *tmp = udisks_block_dup_preferred_device(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockID: {
        char *tmp = udisks_block_dup_id(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockSize:
        return quint64(udisks_block_get_size(blk));
    case Property::kBlockReadOnly:
        return bool(udisks_block_get_read_only(blk));
    case Property::kBlockSymlinks: {
        char **ret = udisks_block_dup_symlinks(blk);
        return Utils::gcharvToQStringList(ret);
    }
    case Property::kBlockHintPartitionable:
        return bool(udisks_block_get_hint_partitionable(blk));
    case Property::kBlockHintSystem:
        return bool(udisks_block_get_hint_system(blk));
    case Property::kBlockHintIgnore:
        return bool(udisks_block_get_hint_ignore(blk));
    case Property::kBlockHintAuto:
        return bool(udisks_block_get_hint_auto(blk));
    case Property::kBlockHintName: {
        char *tmp = udisks_block_dup_hint_name(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockHintIconName: {
        char *tmp = udisks_block_dup_hint_icon_name(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockHintSymbolicIconName: {
        char *tmp = udisks_block_dup_hint_symbolic_icon_name(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockMdRaid: {
        char *tmp = udisks_block_dup_mdraid(blk);
        return Utils::gcharToQString(tmp);
    }
    case Property::kBlockMdRaidMember: {
        char *tmp = udisks_block_dup_mdraid_member(blk);
        return Utils::gcharToQString(tmp);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "not valid for block device");
        return "not valid for block device";
    }
}

QVariant DBlockDevicePrivate::getDriveProperty(Property name) const
{
    UDisksDrive_autoptr drv = getDriveHandler();
    if (!drv) {
        qInfo() << __FUNCTION__ << "NO DRIVE: " << blkObjPath;
        lastError = DeviceError::kUserErrorNoDriver;
        return "";
    }

    switch (name) {
    case Property::kDriveConnectionBus: {
        char *tmp = udisks_drive_dup_connection_bus(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveRemovable:
        return bool(udisks_drive_get_removable(drv));
    case Property::kDriveEjectable:
        return bool(udisks_drive_get_ejectable(drv));
    case Property::kDriveSeat: {
        char *tmp = udisks_drive_dup_seat(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveMedia: {
        char *tmp = udisks_drive_dup_media(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveMediaCompatibility: {
        char **ret = udisks_drive_dup_media_compatibility(drv);
        return Utils::gcharvToQStringList(ret);
    }
    case Property::kDriveMediaRemovable:
        return bool(udisks_drive_get_media_removable(drv));
    case Property::kDriveMediaAvailable:
        return bool(udisks_drive_get_media_available(drv));
    case Property::kDriveMediaChangeDetected:
        return bool(udisks_drive_get_media_change_detected(drv));
    case Property::kDriveTimeDetected:
        return quint64(udisks_drive_get_time_detected(drv));
    case Property::kDriveTimeMediaDetected:
        return quint64(udisks_drive_get_time_media_detected(drv));
    case Property::kDriveSize:
        return quint64(udisks_drive_get_size(drv));
    case Property::kDriveOptical:
        return bool(udisks_drive_get_optical(drv));
    case Property::kDriveOpticalBlank:
        return bool(udisks_drive_get_optical_blank(drv));
    case Property::kDriveOpticalNumTracks:
        return uint(udisks_drive_get_optical_num_tracks(drv));
    case Property::kDriveOpticalNumAudioTracks:
        return uint(udisks_drive_get_optical_num_audio_tracks(drv));
    case Property::kDriveOpticalNumDataTracks:
        return uint(udisks_drive_get_optical_num_data_tracks(drv));
    case Property::kDriveOpticalNumSessions:
        return uint(udisks_drive_get_optical_num_sessions(drv));
    case Property::kDriveModel: {
        char *tmp = udisks_drive_dup_model(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveRevision: {
        char *tmp = udisks_drive_dup_revision(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveRotationRate:
        return int(udisks_drive_get_rotation_rate(drv));
    case Property::kDriveSerial: {
        char *tmp = udisks_drive_dup_serial(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveVender: {
        char *tmp = udisks_drive_dup_vendor(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveWWN: {
        char *tmp = udisks_drive_dup_wwn(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveSortKey: {
        char *tmp = udisks_drive_dup_sort_key(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveConfiguration:
        //        return QVariant(udisks_drive_dup_configuration(drv));
        return "";
    case Property::kDriveID: {
        char *tmp = udisks_drive_dup_id(drv);
        return Utils::gcharToQString(tmp);
    }
    case Property::kDriveCanPowerOff:
        return bool(udisks_drive_get_can_power_off(drv));
    case Property::kDriveSiblingID: {
        char *tmp = udisks_drive_dup_sibling_id(drv);
        return Utils::gcharToQString(tmp);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "not valid property for this device");
        return "";
    }
}

QVariant DBlockDevicePrivate::getFileSystemProperty(Property name) const
{
    UDisksFilesystem_autoptr fs = getFilesystemHandler();
    if (!fs) {
        lastError = DeviceError::kUserErrorNotMountable;
        return QVariant();
    }

    switch (name) {
    case Property::kFileSystemMountPoint: {
        char **ret = udisks_filesystem_dup_mount_points(fs);
        return Utils::gcharvToQStringList(ret);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
        return "";
    }
}

QVariant DBlockDevicePrivate::getPartitionProperty(Property name) const
{
    UDisksPartition_autoptr partition = getPartitionHandler();

    if (!partition) {
        qInfo() << __FUNCTION__ << "NO PARTITION: " << blkObjPath;
        lastError = DeviceError::kUserErrorNoPartition;
        return QVariant();
    }

    switch (name) {
    case Property::kPartitionNumber:
        return uint(udisks_partition_get_number(partition));
    case Property::kPartitionType: {
        char *tmp = udisks_partition_dup_type_(partition);
        return Utils::gcharToQString(tmp);
    }
    case Property::kPartitionOffset:
        return quint64(udisks_partition_get_offset(partition));
    case Property::kPartitionSize:
        return quint64(udisks_partition_get_size(partition));
    case Property::kPartitionFlags:
        return quint64(udisks_partition_get_flags(partition));
    case Property::kPartitionName: {
        char *tmp = udisks_partition_dup_uuid(partition);
        return Utils::gcharToQString(tmp);
    }
    case Property::kPartitionUUID: {
        char *tmp = udisks_partition_dup_type_(partition);
        return Utils::gcharToQString(tmp);
    }
    case Property::kPartitionTable: {
        char *tmp = udisks_partition_dup_table(partition);
        return Utils::gcharToQString(tmp);
    }
    case Property::kPartitionIsContainer:
        return bool(udisks_partition_get_is_container(partition));
    case Property::kPartitionIsContained:
        return bool(udisks_partition_get_is_contained(partition));
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
        return "";
    }
}

QVariant DBlockDevicePrivate::getEncryptedProperty(Property name) const
{
    UDisksEncrypted_autoptr encrypted = getEncryptedHandler();

    if (!encrypted) {
        qInfo() << __FUNCTION__ << "NOT ENCRYPTED: " << blkObjPath;
        lastError = DeviceError::kUserErrorNotEncryptable;
        return QVariant();
    }

    switch (name) {
    case Property::kEncryptedChildConfiguration:
        return Utils::castFromGVariant(udisks_encrypted_get_child_configuration(encrypted));
    case Property::kEncryptedCleartextDevice:
        return Utils::gcharToQString(udisks_encrypted_dup_cleartext_device(encrypted));
    case Property::kEncryptedHintEncryptionType:
        return Utils::gcharToQString(udisks_encrypted_dup_hint_encryption_type(encrypted));
    case Property::kEncryptedMetadataSize:
        return quint64(udisks_encrypted_get_metadata_size(encrypted));
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
        return "";
    }
}
