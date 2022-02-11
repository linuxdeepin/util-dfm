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
#include "base/dfmmount_global.h"
#include "base/dfmmountutils.h"

#include <QStorageInfo>
#include <QDebug>

#include <functional>
#include <udisks/udisks.h>

DFM_MOUNT_USE_NS

inline void DFMBlockDevicePrivate::handleErrorAndRelease(CallbackProxy *proxy, bool result, GError *gerr, QString info)
{
    DeviceError err = DeviceError::NoError;
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

void DFMBlockDevicePrivate::mountAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(sourceObj);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    g_autofree char *mountPoint = nullptr;
    bool result = udisks_filesystem_call_mount_finish(fs, &mountPoint, res, &err);
    handleErrorAndRelease(proxy, result, err, mountPoint);   // ignore mount point, which will be notified by onPropertyChanged
};

void DFMBlockDevicePrivate::unmountAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(sourceObj);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_filesystem_call_unmount_finish(fs, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DFMBlockDevicePrivate::renameAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(sourceObj);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_filesystem_call_set_label_finish(fs, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DFMBlockDevicePrivate::ejectAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksDrive *drive = UDISKS_DRIVE(sourceObj);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    DeviceError derr = DeviceError::NoError;
    bool result = udisks_drive_call_eject_finish(drive, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DFMBlockDevicePrivate::powerOffAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksDrive *drive = UDISKS_DRIVE(sourceObj);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_drive_call_power_off_finish(drive, res, &err);
    handleErrorAndRelease(proxy, result, err);
};

void DFMBlockDevicePrivate::lockAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksEncrypted *encrypted = UDISKS_ENCRYPTED(sourceObj);
    Q_ASSERT_X(encrypted, __FUNCTION__, "encrypted is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    bool result = udisks_encrypted_call_lock_finish(encrypted, res, &err);
    handleErrorAndRelease(proxy, result, err);
}

void DFMBlockDevicePrivate::unlockAsyncCallback(GObject *sourceObj, GAsyncResult *res, gpointer userData)
{
    UDisksEncrypted *encrypted = UDISKS_ENCRYPTED(sourceObj);
    Q_ASSERT_X(encrypted, __FUNCTION__, "encrypted is not valid");
    CallbackProxy *proxy = static_cast<CallbackProxy *>(userData);

    GError *err = nullptr;
    g_autofree char *clearTextDev = nullptr;
    bool result = udisks_encrypted_call_unlock_finish(encrypted, &clearTextDev, res, &err);
    handleErrorAndRelease(proxy, result, err, QString(clearTextDev));
}

DFMBlockDevice::DFMBlockDevice(UDisksClient *cli, const QString &udisksObjPath, QObject *parent)
    : DFMDevice(new DFMBlockDevicePrivate(cli, udisksObjPath, this), parent)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }

    using namespace std;
    using namespace std::placeholders;
    registerPath(bind(&DFMBlockDevicePrivate::path, dp));
    registerMount(bind(&DFMBlockDevicePrivate::mount, dp, _1));
    registerMountAsync(bind(&DFMBlockDevicePrivate::mountAsync, dp, _1, _2));
    registerUnmount(bind(&DFMBlockDevicePrivate::unmount, dp, _1));
    registerUnmountAsync(bind(&DFMBlockDevicePrivate::unmountAsync, dp, _1, _2));
    registerRename(bind(&DFMBlockDevicePrivate::rename, dp, _1, _2));
    registerRenameAsync(bind(&DFMBlockDevicePrivate::renameAsync, dp, _1, _2, _3));
    registerMountPoint(bind(&DFMBlockDevicePrivate::mountPoint, dp));
    registerFileSystem(bind(&DFMBlockDevicePrivate::fileSystem, dp));
    registerSizeTotal(bind(&DFMBlockDevicePrivate::sizeTotal, dp));
    registerSizeUsage(bind(&DFMBlockDevicePrivate::sizeUsage, dp));
    registerSizeFree(bind(&DFMBlockDevicePrivate::sizeFree, dp));
    registerDeviceType(bind(&DFMBlockDevicePrivate::deviceType, dp));
    registerGetProperty(bind(&DFMBlockDevicePrivate::getProperty, dp, _1));
    registerDisplayName(bind(&DFMBlockDevicePrivate::displayName, dp));
}

DFMBlockDevice::~DFMBlockDevice()
{
}

bool DFMBlockDevice::eject(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->eject(opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DFMBlockDevice::ejectAsync(const QVariantMap &opts, Callback1 cb)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->ejectAsync(opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

bool DFMBlockDevice::powerOff(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->powerOff(opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DFMBlockDevice::powerOffAsync(const QVariantMap &opts, Callback1 cb)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        dp->powerOffAsync(opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

bool DFMBlockDevice::lock(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->lock(opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DFMBlockDevice::lockAsync(const QVariantMap &opts, Callback1 cb)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        dp->lockAsync(opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

bool DFMBlockDevice::unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        return dp->unlock(passwd, clearTextDev, opts);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
        return false;
    }
}

void DFMBlockDevice::unlockAsync(const QString &passwd, const QVariantMap &opts, Callback2 cb)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp) {
        dp->unlockAsync(passwd, opts, cb);
    } else {
        qCritical() << "DP IS NULL: " << __PRETTY_FUNCTION__;
    }
}

QStringList DFMBlockDevice::mountPoints() const
{
    return getProperty(Property::FileSystemMountPoint).toStringList();
}

QString DFMBlockDevice::device() const
{
    return getProperty(Property::BlockDevice).toString();
}

QString DFMBlockDevice::drive() const
{
    return getProperty(Property::BlockDrive).toString();
}

QString DFMBlockDevice::idLabel() const
{
    return getProperty(Property::BlockIDLabel).toString();
}

bool DFMBlockDevice::removable() const
{
    return getProperty(Property::DriveRemovable).toBool();
}

bool DFMBlockDevice::optical() const
{
    return getProperty(Property::DriveOptical).toBool();
}

bool DFMBlockDevice::opticalBlank() const
{
    return getProperty(Property::DriveOpticalBlank).toBool();
}

QStringList DFMBlockDevice::mediaCompatibility() const
{
    return getProperty(Property::DriveMediaCompatibility).toStringList();
}

bool DFMBlockDevice::canPowerOff() const
{
    return getProperty(Property::DriveCanPowerOff).toBool();
}

bool DFMBlockDevice::ejectable() const
{
    return getProperty(Property::DriveEjectable).toBool();
}

bool DFMBlockDevice::isEncrypted() const
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->encryptedHandler != nullptr : false;
}

bool DFMBlockDevice::hasFileSystem() const
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->fileSystemHandler != nullptr : false;
}

bool DFMBlockDevice::hasPartitionTable() const
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->partitionTabHandler != nullptr : false;
}

bool DFMBlockDevice::hasPartition() const
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->partitionHandler != nullptr : false;
}

bool DFMBlockDevice::isLoopDevice() const
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->loopHandler != nullptr : false;
}

bool DFMBlockDevice::hintIgnore() const
{
    return getProperty(Property::BlockHintIgnore).toBool();
}

bool DFMBlockDevice::hintSystem() const
{
    return getProperty(Property::BlockHintSystem).toBool();
}

PartitionType DFMBlockDevice::partitionEType() const
{
    auto typestr = partitionType();
    if (typestr.isEmpty())
        return PartitionType::PartitionTypeNotFound;
    bool ok = false;
    int type = typestr.toInt(&ok, 16);
    if (ok) {
        if (type >= static_cast<int>(PartitionType::MbrEmpty)
            && type <= static_cast<int>(PartitionType::MbrBBT))
            return static_cast<PartitionType>(type);
        else
            return PartitionType::PartitionTypeNotFound;
    } else {
        return Utils::getPartitionTypeByGuid(typestr);
    }
}

QString DFMBlockDevice::partitionType() const
{
    return getProperty(Property::PartitionType).toString();
}

bool DFMBlockDevice::hasBlock() const
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->blockHandler != nullptr : false;
}

DFMBlockDevicePrivate::DFMBlockDevicePrivate(UDisksClient *cli, const QString &blkObjPath, DFMBlockDevice *qq)
    : DFMDevicePrivate(qq), blkObjPath(blkObjPath), client(cli)
{
    init();
}

DFMBlockDevicePrivate::~DFMBlockDevicePrivate()
{
    auto gunref = [](void *gobj) {
        if (gobj)
            g_object_unref(gobj);
    };
    gunref(driveHandler);
    gunref(fileSystemHandler);
    gunref(blockHandler);
    gunref(partitionHandler);
    gunref(encryptedHandler);
    gunref(partitionTabHandler);
    gunref(loopHandler);
}

QString DFMBlockDevicePrivate::path() const
{
    return blkObjPath;
}

QString DFMBlockDevicePrivate::mount(const QVariantMap &opts)
{
    warningIfNotInMain();

    if (!fileSystemHandler) {
        lastError = DeviceError::UserErrorNotMountable;
        return "";
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::UDisksErrorAlreadyMounted;
        return mpts.first();
    }

    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    char *mountPoint = nullptr;
    bool mounted = udisks_filesystem_call_mount_sync(fileSystemHandler, gopts, &mountPoint, nullptr, &err);

    handleErrorAndRelase(err);
    QString ret;
    if (mounted && mountPoint) {
        ret = mountPoint;
        g_free(mountPoint);
    }
    return ret;
}

void DFMBlockDevicePrivate::mountAsync(const QVariantMap &opts, Callback2 cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!fileSystemHandler) {
        lastError = DeviceError::UserErrorNotMountable;
        if (proxy) {
            proxy->cbWithInfo(false, lastError, "");
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::UDisksErrorAlreadyMounted;
        if (proxy) {
            proxy->cbWithInfo(true, lastError, mpts.first()); // when it's already mounted, return true but report an error refer to `already mounted`.
            delete proxy;
        }
        return;
    }

    // mount device async
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_filesystem_call_mount(fileSystemHandler, gopts, nullptr, mountAsyncCallback, proxy);
}

bool DFMBlockDevicePrivate::unmount(const QVariantMap &opts)
{
    warningIfNotInMain();

    if (!fileSystemHandler) {
        lastError = DeviceError::UserErrorNotMountable;
        return true;   // since device is not mountable, so we just return true here
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = DeviceError::UDisksErrorNotMounted;
        return true;   // since it's not mounted, then this invocation returns true
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;
    bool result = udisks_filesystem_call_unmount_sync(fileSystemHandler, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelase(err);
    return false;
}

void DFMBlockDevicePrivate::unmountAsync(const QVariantMap &opts, Callback1 cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!fileSystemHandler) {
        lastError = DeviceError::UserErrorNotMountable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = DeviceError::UDisksErrorNotMounted;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    // start unmount device async
    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_filesystem_call_unmount(fileSystemHandler, gopts, nullptr, unmountAsyncCallback, proxy);
}

bool DFMBlockDevicePrivate::rename(const QString &newName, const QVariantMap &opts)
{
    warningIfNotInMain();

    if (!fileSystemHandler) {
        lastError = DeviceError::UserErrorNotMountable;
        return false;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::UDisksErrorAlreadyMounted;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    const char *label = newName.toStdString().c_str();
    GError *err = nullptr;

    bool result = udisks_filesystem_call_set_label_sync(fileSystemHandler, label, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelase(err);
    return false;
}

void DFMBlockDevicePrivate::renameAsync(const QString &newName, const QVariantMap &opts, Callback1 cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!fileSystemHandler) {
        lastError = DeviceError::UserErrorNotMountable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::UDisksErrorAlreadyMounted;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    const char *label = newName.toStdString().c_str();
    udisks_filesystem_call_set_label(fileSystemHandler, label, gopts, nullptr, renameAsyncCallback, proxy);
}

bool DFMBlockDevicePrivate::eject(const QVariantMap &opts)
{
    warningIfNotInMain();

    bool ejectable = q->getProperty(Property::DriveEjectable).toBool();
    if (!ejectable) {
        lastError = DeviceError::UserErrorNotEjectable;
        return false;
    }

    if (!driveHandler) {
        lastError = DeviceError::UserErrorNoDriver;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;

    bool result = udisks_drive_call_eject_sync(driveHandler, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelase(err);
    return false;
}

void DFMBlockDevicePrivate::ejectAsync(const QVariantMap &opts, Callback1 cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    bool ejectable = q->getProperty(Property::DriveEjectable).toBool();
    if (!ejectable) {
        lastError = DeviceError::UserErrorNotEjectable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    if (!driveHandler) {
        lastError = DeviceError::UserErrorNoDriver;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_drive_call_eject(driveHandler, gopts, nullptr, ejectAsyncCallback, proxy);
}

bool DFMBlockDevicePrivate::powerOff(const QVariantMap &opts)
{
    warningIfNotInMain();

    if (!driveHandler) {
        lastError = DeviceError::UserErrorNoDriver;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;
    bool result = udisks_drive_call_power_off_sync(driveHandler, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelase(err);
    return false;
}

void DFMBlockDevicePrivate::powerOffAsync(const QVariantMap &opts, Callback1 cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!driveHandler) {
        lastError = DeviceError::UserErrorNoDriver;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_drive_call_power_off(driveHandler, gopts, nullptr, powerOffAsyncCallback, proxy);
}

bool DFMBlockDevicePrivate::lock(const QVariantMap &opts)
{
    warningIfNotInMain();

    if (!encryptedHandler) {
        lastError = DeviceError::UserErrorNotEncryptable;
        return false;
    }

    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    bool result = udisks_encrypted_call_lock_sync(encryptedHandler, gopts, nullptr, &err);
    if (result)
        return true;

    handleErrorAndRelase(err);
    return false;
}

void DFMBlockDevicePrivate::lockAsync(const QVariantMap &opts, Callback1 cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!encryptedHandler) {
        lastError = DeviceError::UserErrorNotEncryptable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_encrypted_call_lock(encryptedHandler, gopts, nullptr, lockAsyncCallback, proxy);
}

bool DFMBlockDevicePrivate::unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts)
{
    warningIfNotInMain();

    if (!encryptedHandler) {
        lastError = DeviceError::UserErrorNotEncryptable;
        return false;
    }

    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    char *clearDev = nullptr;
    bool result = udisks_encrypted_call_unlock_sync(encryptedHandler, passwd.toStdString().c_str(), gopts, &clearDev, nullptr, &err);
    if (result) {
        clearTextDev = QString(clearDev);
        g_free(clearDev);
        return true;
    }

    handleErrorAndRelase(err);
    return false;
}

void DFMBlockDevicePrivate::unlockAsync(const QString &passwd, const QVariantMap &opts, Callback2 cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!encryptedHandler) {
        lastError = DeviceError::UserErrorNotEncryptable;
        if (proxy) {
            proxy->cbWithInfo(false, lastError, QString());
            delete proxy;
        }
        return;
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_encrypted_call_unlock(encryptedHandler, passwd.toStdString().c_str(), gopts, nullptr, unlockAsyncCallback, proxy);
}

inline void DFMBlockDevicePrivate::handleErrorAndRelase(GError *err)
{
    if (err) {
        lastError = Utils::castFromGError(err);
        g_error_free(err);
    }
}

QString DFMBlockDevicePrivate::mountPoint() const
{
    auto mpts = q->getProperty(Property::FileSystemMountPoint).toStringList();
    return mpts.isEmpty() ? QString() : mpts.first();
}

QString DFMBlockDevicePrivate::fileSystem() const
{
    return getProperty(Property::BlockIDType).toString();
}

qint64 DFMBlockDevicePrivate::sizeTotal() const
{
    return q->getProperty(Property::BlockSize).toLongLong();
}

qint64 DFMBlockDevicePrivate::sizeUsage() const
{
    return sizeTotal() - sizeFree();
}

qint64 DFMBlockDevicePrivate::sizeFree() const
{
    auto mpts = q->getProperty(Property::FileSystemMountPoint).toStringList();
    if (mpts.isEmpty()) {
        //        lastError = MountError::NotMounted;
        qWarning() << __FUNCTION__ << "NOT MOUNTED: " << blkObjPath;
        return 0;
    }
    auto mpt = mpts.first();
    QStorageInfo info(mpt);
    return info.bytesAvailable();
}

DeviceType DFMBlockDevicePrivate::deviceType() const
{
    return DeviceType::BlockDevice;
}

QVariant DFMBlockDevicePrivate::getProperty(Property name) const
{
    if (name > Property::BlockProperty && name < Property::BlockPropertyEND)
        return getBlockProperty(name);
    else if (name > Property::DriveProperty && name < Property::DrivePropertyEND)
        return getDriveProperty(name);
    else if (name > Property::FileSystemProperty && name < Property::FileSystemPropertyEND)
        return getFileSystemProperty(name);
    else if (name > Property::PartitionProperty && name < Property::PartitionPropertyEND)
        return getPartitionProperty(name);
    else if (name > Property::EncryptedProperty && name < Property::EncryptedPropertyEnd)
        return getEncryptedProperty(name);

    //    Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
}

QString DFMBlockDevicePrivate::displayName() const
{
    return getProperty(Property::BlockIDLabel).toString();
}

QVariant DFMBlockDevicePrivate::getBlockProperty(Property name) const
{
    if (!blockHandler) {
        qWarning() << __FUNCTION__ << "NO BLOCK: " << blkObjPath;
        lastError = DeviceError::UserErrorNoBlock;
        return QVariant();
    }

    // make sure we can safely get the properties in cross-thread cases: so we use DUP rather than GET when DUP can be used.
    // but we shall release the objects by calling g_free for char * or g_strfreev for char ** funcs.
    switch (name) {
    case Property::BlockConfiguration:
        //                return udisks_block_dup_configuration(blockHandler); TODO
        return "";
    case Property::BlockCryptoBackingDevice: {
        char *tmp = udisks_block_dup_crypto_backing_device(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockDevice: {
        char *tmp = udisks_block_dup_device(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockDrive: {
        char *tmp = udisks_block_dup_drive(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockIDLabel: {
        char *tmp = udisks_block_dup_id_label(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockIDType: {
        char *tmp = udisks_block_dup_id_type(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockIDUsage: {
        char *tmp = udisks_block_dup_id_usage(blockHandler);
        QString ret = Utils::gcharToQString(tmp);
        return ret.toLongLong();
    }
    case Property::BlockIDUUID: {
        char *tmp = udisks_block_dup_id_uuid(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockIDVersion: {
        char *tmp = udisks_block_dup_id_version(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockDeviceNumber:
        return quint64(udisks_block_get_device_number(blockHandler));
    case Property::BlockPreferredDevice: {
        char *tmp = udisks_block_dup_preferred_device(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockID: {
        char *tmp = udisks_block_dup_id(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockSize:
        return quint64(udisks_block_get_size(blockHandler));
    case Property::BlockReadOnly:
        return bool(udisks_block_get_read_only(blockHandler));
    case Property::BlockSymlinks: {
        char **ret = udisks_block_dup_symlinks(blockHandler);
        return Utils::gcharvToQStringList(ret);
    }
    case Property::BlockHintPartitionable:
        return bool(udisks_block_get_hint_partitionable(blockHandler));
    case Property::BlockHintSystem:
        return bool(udisks_block_get_hint_system(blockHandler));
    case Property::BlockHintIgnore:
        return bool(udisks_block_get_hint_ignore(blockHandler));
    case Property::BlockHintAuto:
        return bool(udisks_block_get_hint_auto(blockHandler));
    case Property::BlockHintName: {
        char *tmp = udisks_block_dup_hint_name(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockHintIconName: {
        char *tmp = udisks_block_dup_hint_icon_name(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockHintSymbolicIconName: {
        char *tmp = udisks_block_dup_hint_symbolic_icon_name(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockMdRaid: {
        char *tmp = udisks_block_dup_mdraid(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::BlockMdRaidMember: {
        char *tmp = udisks_block_dup_mdraid_member(blockHandler);
        return Utils::gcharToQString(tmp);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "not valid for block device");
        return "not valid for block device";
    }
}

QVariant DFMBlockDevicePrivate::getDriveProperty(Property name) const
{
    if (!driveHandler) {
        qWarning() << __FUNCTION__ << "NO DRIVE: " << blkObjPath;
        lastError = DeviceError::UserErrorNoDriver;
        return "";
    }
    switch (name) {
    case Property::DriveConnectionBus: {
        char *tmp = udisks_drive_dup_connection_bus(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveRemovable:
        return bool(udisks_drive_get_removable(driveHandler));
    case Property::DriveEjectable:
        return bool(udisks_drive_get_ejectable(driveHandler));
    case Property::DriveSeat: {
        char *tmp = udisks_drive_dup_seat(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveMedia: {
        char *tmp = udisks_drive_dup_media(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveMediaCompatibility: {
        char **ret = udisks_drive_dup_media_compatibility(driveHandler);
        return Utils::gcharvToQStringList(ret);
    }
    case Property::DriveMediaRemovable:
        return bool(udisks_drive_get_media_removable(driveHandler));
    case Property::DriveMediaAvailable:
        return bool(udisks_drive_get_media_available(driveHandler));
    case Property::DriveMediaChangeDetected:
        return bool(udisks_drive_get_media_change_detected(driveHandler));
    case Property::DriveTimeDetected:
        return quint64(udisks_drive_get_time_detected(driveHandler));
    case Property::DriveTimeMediaDetected:
        return quint64(udisks_drive_get_time_media_detected(driveHandler));
    case Property::DriveSize:
        return quint64(udisks_drive_get_size(driveHandler));
    case Property::DriveOptical:
        return bool(udisks_drive_get_optical(driveHandler));
    case Property::DriveOpticalBlank:
        return bool(udisks_drive_get_optical_blank(driveHandler));
    case Property::DriveOpticalNumTracks:
        return uint(udisks_drive_get_optical_num_tracks(driveHandler));
    case Property::DriveOpticalNumAudioTracks:
        return uint(udisks_drive_get_optical_num_audio_tracks(driveHandler));
    case Property::DriveOpticalNumDataTracks:
        return uint(udisks_drive_get_optical_num_data_tracks(driveHandler));
    case Property::DriveOpticalNumSessions:
        return uint(udisks_drive_get_optical_num_sessions(driveHandler));
    case Property::DriveModel: {
        char *tmp = udisks_drive_dup_model(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveRevision: {
        char *tmp = udisks_drive_dup_revision(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveRotationRate:
        return int(udisks_drive_get_rotation_rate(driveHandler));
    case Property::DriveSerial: {
        char *tmp = udisks_drive_dup_serial(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveVender: {
        char *tmp = udisks_drive_dup_vendor(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveWWN: {
        char *tmp = udisks_drive_dup_wwn(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveSortKey: {
        char *tmp = udisks_drive_dup_sort_key(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveConfiguration:
        //        return QVariant(udisks_drive_dup_configuration(driveHandler));
        return "";
    case Property::DriveID: {
        char *tmp = udisks_drive_dup_id(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::DriveCanPowerOff:
        return bool(udisks_drive_get_can_power_off(driveHandler));
    case Property::DriveSiblingID: {
        char *tmp = udisks_drive_dup_sibling_id(driveHandler);
        return Utils::gcharToQString(tmp);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "not valid property for this device");
        return "";
    }
}

QVariant DFMBlockDevicePrivate::getFileSystemProperty(Property name) const
{
    if (!fileSystemHandler) {
        lastError = DeviceError::UserErrorNotMountable;
        return QVariant();
    }

    switch (name) {
    case Property::FileSystemMountPoint: {
        char **ret = udisks_filesystem_dup_mount_points(fileSystemHandler);
        return Utils::gcharvToQStringList(ret);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
    }
}

QVariant DFMBlockDevicePrivate::getPartitionProperty(Property name) const
{
    if (!partitionHandler) {
        qWarning() << __FUNCTION__ << "NO PARTITION: " << blkObjPath;
        lastError = DeviceError::UserErrorNoPartition;
        return QVariant();
    }

    switch (name) {
    case Property::PartitionNumber:
        return uint(udisks_partition_get_number(partitionHandler));
    case Property::PartitionType: {
        char *tmp = udisks_partition_dup_type_(partitionHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::PartitionOffset:
        return quint64(udisks_partition_get_offset(partitionHandler));
    case Property::PartitionSize:
        return quint64(udisks_partition_get_size(partitionHandler));
    case Property::PartitionFlags:
        return quint64(udisks_partition_get_flags(partitionHandler));
    case Property::PartitionName: {
        char *tmp = udisks_partition_dup_uuid(partitionHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::PartitionUUID: {
        char *tmp = udisks_partition_dup_type_(partitionHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::PartitionTable: {
        char *tmp = udisks_partition_dup_table(partitionHandler);
        return Utils::gcharToQString(tmp);
    }
    case Property::PartitionIsContainer:
        return bool(udisks_partition_get_is_container(partitionHandler));
    case Property::PartitionIsContained:
        return bool(udisks_partition_get_is_contained(partitionHandler));
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
    }
}

QVariant DFMBlockDevicePrivate::getEncryptedProperty(Property name) const
{
    if (!encryptedHandler) {
        qWarning() << __FUNCTION__ << "NOT ENCRYPTED: " << blkObjPath;
        lastError = DeviceError::UserErrorNotEncryptable;
        return QVariant();
    }

    switch (name) {
    case Property::EncryptedChildConfiguration:
        return Utils::castFromGVariant(udisks_encrypted_get_child_configuration(encryptedHandler));
    case Property::EncryptedCleartextDevice:
        return Utils::gcharToQString(udisks_encrypted_dup_cleartext_device(encryptedHandler));
    case Property::EncryptedHintEncryptionType:
        return Utils::gcharToQString(udisks_encrypted_dup_hint_encryption_type(encryptedHandler));
    case Property::EncryptedMetadataSize:
        return quint64(udisks_encrypted_get_metadata_size(encryptedHandler));
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
    }
}

void DFMBlockDevicePrivate::init()
{
    // block until all pending message been recieved.
    // make sure that we can obtain the handlers if the device is created just recently
    udisks_client_settle(client);
    std::string str = blkObjPath.toStdString();
    UDisksObject *blkObj = udisks_client_peek_object(client, str.c_str());
    if (!blkObj)
        return;

    blockHandler = udisks_object_get_block(blkObj);
    fileSystemHandler = udisks_object_get_filesystem(blkObj);
    partitionHandler = udisks_object_get_partition(blkObj);
    encryptedHandler = udisks_object_get_encrypted(blkObj);
    partitionTabHandler = udisks_object_get_partition_table(blkObj);
    loopHandler = udisks_object_get_loop(blkObj);

    if (blockHandler) {
        // must be freed with g_object_unref
        driveHandler = udisks_client_get_drive_for_block(client, blockHandler);
    }
}
