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
#include "base/dfmmountdefines.h"
#include "base/dfmmountutils.h"

#include <QFuture>
#include <QStorageInfo>

#include <functional>
#include <udisks/udisks.h>

DFM_MOUNT_USE_NS

#define UDISKS_ERR_DOMAIN "udisks-error-quark"

// only for parse the return value of *_DUP_*, do not use it with *_GET_*
static QString charToQString(char *tmp) {
    if (!tmp)
        return QString();
    QString ret(tmp);
    g_free(tmp);
    return ret;
}

// only for parse the return value of *_DUP_*, do not use it with *_GET_*
static QStringList charToQStringList(char **tmp) {
    QStringList ret;
    int next = 0;
    while (tmp && tmp[next]) {
        ret << QString(tmp[next]);
        next += 1;
    }
    if (tmp)
        g_strfreev(tmp);
    return ret;
}

static void mountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    char *mountPoint = nullptr;
    GError *err = nullptr;

    UDisksFilesystem *fs = UDISKS_FILESYSTEM(source_object);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");

    DeviceError derr = DeviceError::NoError;
    bool result = udisks_filesystem_call_mount_finish(fs, &mountPoint, res, &err);
    if (!result) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            derr = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    if (mountPoint)
        g_free(mountPoint);
    CallbackProxy *proxy = static_cast<CallbackProxy *>(user_data);
    if (proxy) {
        proxy->cb(result, derr);
        delete proxy;
    }
};

static void unmountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(source_object);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");

    DeviceError derr = DeviceError::NoError;
    GError *err = nullptr;
    bool result = udisks_filesystem_call_unmount_finish(fs, res, &err);
    if (!result) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            derr = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    CallbackProxy *proxy = static_cast<CallbackProxy *>(user_data);
    if (proxy) {
        proxy->cb(result, derr);
        delete proxy;
    }
};

static void renameAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    UDisksFilesystem *fs = UDISKS_FILESYSTEM(source_object);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");

    DeviceError derr = DeviceError::NoError;
    GError *err = nullptr;
    bool result = udisks_filesystem_call_set_label_finish(fs, res, &err);
    if (!result) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            derr = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    CallbackProxy *proxy = static_cast<CallbackProxy *>(user_data);
    if (proxy) {
        proxy->cb(result, derr);
        delete proxy;
    }
};

static void ejectAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    UDisksDrive *drive = UDISKS_DRIVE(source_object);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");

    GError *err = nullptr;
    DeviceError derr = DeviceError::NoError;
    bool result = udisks_drive_call_eject_finish(drive, res, &err);
    if (!result) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            derr = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    CallbackProxy *proxy = static_cast<CallbackProxy *>(user_data);
    if (proxy) {
        proxy->cb(result, derr);
        delete proxy;
    }
};

static void powerOffAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    UDisksDrive *drive = UDISKS_DRIVE(source_object);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");

    GError *err = nullptr;
    DeviceError derr = DeviceError::NoError;
    bool result = udisks_drive_call_power_off_finish(drive, res, &err);
    if (!result) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            derr = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    CallbackProxy *proxy = static_cast<CallbackProxy *>(user_data);
    if (proxy) {
        proxy->cb(result, derr);
        delete proxy;
    }
};

DFMBlockDevice::DFMBlockDevice(UDisksClient *cli, const QString &udisksObjPath, QObject *parent)
    : DFMDevice(new DFMBlockDevicePrivate(cli, udisksObjPath, this), parent)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    registerPath(std::bind(&DFMBlockDevicePrivate::path, dp));
    registerMount(std::bind(&DFMBlockDevicePrivate::mount, dp, std::placeholders::_1));
    registerMountAsync(std::bind(&DFMBlockDevicePrivate::mountAsync, dp, std::placeholders::_1, std::placeholders::_2));
    registerUnmount(std::bind(&DFMBlockDevicePrivate::unmount, dp, std::placeholders::_1));
    registerUnmountAsync(std::bind(&DFMBlockDevicePrivate::unmountAsync, dp, std::placeholders::_1, std::placeholders::_2));
    registerRename(std::bind(&DFMBlockDevicePrivate::rename, dp, std::placeholders::_1, std::placeholders::_2));
    registerRenameAsync(std::bind(&DFMBlockDevicePrivate::renameAsync, dp, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    registerMountPoint(std::bind(&DFMBlockDevicePrivate::mountPoint, dp));
    registerFileSystem(std::bind(&DFMBlockDevicePrivate::fileSystem, dp));
    registerSizeTotal(std::bind(&DFMBlockDevicePrivate::sizeTotal, dp));
    registerSizeUsage(std::bind(&DFMBlockDevicePrivate::sizeUsage, dp));
    registerSizeFree(std::bind(&DFMBlockDevicePrivate::sizeFree, dp));
    registerDeviceType(std::bind(&DFMBlockDevicePrivate::deviceType, dp));
    registerGetProperty(std::bind(&DFMBlockDevicePrivate::getProperty, dp, std::placeholders::_1));
}

DFMBlockDevice::~DFMBlockDevice()
{
    qDebug() << __FUNCTION__ << "is released: " << path();
}

bool DFMBlockDevice::eject(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->eject(opts) : false;
}

void DFMBlockDevice::ejectAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp)
        return dp->ejectAsync(opts, cb);
}

bool DFMBlockDevice::powerOff(const QVariantMap &opts)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp ? dp->powerOff(opts) : false;
}

void DFMBlockDevice::powerOffAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp)
        return dp->powerOffAsync(opts, cb);
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

DFMBlockDevicePrivate::DFMBlockDevicePrivate(UDisksClient *cli, const QString &blkObjPath, DFMBlockDevice *qq)
    : DFMDevicePrivate(qq), blkObjPath(blkObjPath)
{
    init(cli);
}

QString DFMBlockDevicePrivate::path() const
{
    return blkObjPath;
}

QString DFMBlockDevicePrivate::mount(const QVariantMap &opts)
{
    warningIfNotInMain();

    if (!fileSystemHandler) {
        lastError = DeviceError::NotMountable;
        return "";
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::AlreadyMounted;
        return mpts.first();
    }

    GError *err = nullptr;
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    char *mountPoint = nullptr;
    bool mounted = udisks_filesystem_call_mount_sync(fileSystemHandler, gopts, &mountPoint, nullptr, &err);

    if (err) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            lastError = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }

    QString ret;
    if (mounted && mountPoint) {
        ret = mountPoint;
        g_free(mountPoint);
    }

    return ret;
}

void DFMBlockDevicePrivate::mountAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!fileSystemHandler) {
        lastError = DeviceError::NotMountable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::AlreadyMounted;
        if (proxy) {
            proxy->cb(false, lastError);
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
        lastError = DeviceError::NotMountable;
        return true; // since device is not mountable, so we just return true here
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = DeviceError::NotMounted;
        return true; // since it's not mounted, then this invocation returns true
    }

    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;
    bool result = udisks_filesystem_call_unmount_sync(fileSystemHandler, gopts, nullptr, &err);
    if (result)
        return true;

    if (err) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            lastError = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }

    return false;
}

void DFMBlockDevicePrivate::unmountAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!fileSystemHandler) {
        lastError = DeviceError::NotMountable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = DeviceError::NotMounted;
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
        lastError = DeviceError::NotMountable;
        return false;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::AlreadyMounted;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    const char *label = newName.toStdString().c_str();
    GError *err = nullptr;

    bool result = udisks_filesystem_call_set_label_sync(fileSystemHandler, label, gopts, nullptr, &err);
    if (result)
        return true;

    if (err) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            lastError = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    return false;
}

void DFMBlockDevicePrivate::renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCb cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!fileSystemHandler) {
        lastError = DeviceError::NotMountable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = DeviceError::AlreadyMounted;
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
        lastError = DeviceError::NotEjectable;
        return false;
    }

    if (!driveHandler) {
        lastError = DeviceError::NoDriver;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;

    bool result = udisks_drive_call_eject_sync(driveHandler, gopts, nullptr, &err);
    if (result)
        return true;

    if (err) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            lastError = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    return false;
}

void DFMBlockDevicePrivate::ejectAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    bool ejectable = q->getProperty(Property::DriveEjectable).toBool();
    if (!ejectable) {
        lastError = DeviceError::NotEjectable;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }

    if (!driveHandler) {
        lastError = DeviceError::NoDriver;
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
        lastError = DeviceError::NoDriver;
        return false;
    }

    // construct the options
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    GError *err = nullptr;
    bool result = udisks_drive_call_power_off_sync(driveHandler, gopts, nullptr, &err);
    if (result)
        return true;

    if (err) {
        if (strcmp(g_quark_to_string(err->domain), UDISKS_ERR_DOMAIN) == 0)
            lastError = static_cast<DeviceError>(err->code + 1);
        else
            qDebug() << __FUNCTION__ << err->code << err->message << g_quark_to_string(err->domain);
        g_error_free(err);
    }
    return false;
}

void DFMBlockDevicePrivate::powerOffAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
    CallbackProxy *proxy = cb ? new CallbackProxy(cb) : nullptr;
    if (!driveHandler) {
        lastError = DeviceError::NoDriver;
        if (proxy) {
            proxy->cb(false, lastError);
            delete proxy;
        }
        return;
    }
    GVariant *gopts = Utils::castFromQVariantMap(opts);
    udisks_drive_call_power_off(driveHandler, gopts, nullptr, powerOffAsyncCallback, proxy);
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
        qWarning() << "need to mount first to get the availabel size";
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

    Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
}

QVariant DFMBlockDevicePrivate::getBlockProperty(Property name) const
{
    Q_ASSERT_X(blockHandler, __PRETTY_FUNCTION__, "not valid");

    // make sure we can safely get the properties in cross-thread cases: so we use DUP rather than GET when DUP can be used.
    // but we shall release the objects by calling g_free for char * or g_strfreev for char ** funcs.
    switch (name) {
    case Property::BlockConfiguration:
        //        return udisks_block_dup_configuration(blockHandler);
        return "";
    case Property::BlockCryptoBackingDevice: {
        char *tmp = udisks_block_dup_crypto_backing_device(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockDevice: {
        char *tmp = udisks_block_dup_device(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockDrive: {
        char *tmp = udisks_block_dup_drive(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockIDLabel: {
        char *tmp = udisks_block_dup_id_label(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockIDType: {
        char *tmp = udisks_block_dup_id_type(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockIDUsage: {
        char *tmp = udisks_block_dup_id_usage(blockHandler);
        QString ret = charToQString(tmp);
        return ret.toLongLong();
    }
    case Property::BlockIDUUID: {
        char *tmp = udisks_block_dup_id_uuid(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockIDVersion: {
        char *tmp = udisks_block_dup_id_version(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockDeviceNumber:
        return quint64(udisks_block_get_device_number(blockHandler));
    case Property::BlockPreferredDevice: {
        char *tmp = udisks_block_dup_preferred_device(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockID: {
        char *tmp = udisks_block_dup_id(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockSize:
        return quint64(udisks_block_get_size(blockHandler));
    case Property::BlockReadOnly:
        return bool(udisks_block_get_read_only(blockHandler));
    case Property::BlockSymlinks: {
        char **ret = udisks_block_dup_symlinks(blockHandler);
        return charToQStringList(ret);
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
        return charToQString(tmp);
    }
    case Property::BlockHintIconName: {
        char *tmp = udisks_block_dup_hint_icon_name(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockHintSymbolicIconName: {
        char *tmp = udisks_block_dup_hint_symbolic_icon_name(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockMdRaid: {
        char *tmp = udisks_block_dup_mdraid(blockHandler);
        return charToQString(tmp);
    }
    case Property::BlockMdRaidMember: {
        char *tmp = udisks_block_dup_mdraid_member(blockHandler);
        return charToQString(tmp);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "not valid for block device");
        return "not valid for block device";
    }
}

QVariant DFMBlockDevicePrivate::getDriveProperty(Property name) const
{
    if (!driveHandler) {
        qWarning() << "this device do not have a physical drive";
        return "";
    }
    switch (name) {
    case Property::DriveConnectionBus: {
        char *tmp = udisks_drive_dup_connection_bus(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveRemovable:
        return bool(udisks_drive_get_removable(driveHandler));
    case Property::DriveEjectable:
        return bool(udisks_drive_get_ejectable(driveHandler));
    case Property::DriveSeat: {
        char *tmp = udisks_drive_dup_seat(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveMedia: {
        char *tmp = udisks_drive_dup_media(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveMediaCompatibility: {
        char **ret = udisks_drive_dup_media_compatibility(driveHandler);
        return charToQStringList(ret);
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
        return charToQString(tmp);
    }
    case Property::DriveRevision: {
        char *tmp = udisks_drive_dup_revision(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveRotationRate:
        return int(udisks_drive_get_rotation_rate(driveHandler));
    case Property::DriveSerial: {
        char *tmp = udisks_drive_dup_serial(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveVender: {
        char *tmp = udisks_drive_dup_vendor(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveWWN: {
        char *tmp = udisks_drive_dup_wwn(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveSortKey: {
        char *tmp = udisks_drive_dup_sort_key(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveConfiguration:
        //        return QVariant(udisks_drive_dup_configuration(driveHandler));
        return "";
    case Property::DriveID: {
        char *tmp = udisks_drive_dup_id(driveHandler);
        return charToQString(tmp);
    }
    case Property::DriveCanPowerOff:
        return bool(udisks_drive_get_can_power_off(driveHandler));
    case Property::DriveSiblingID: {
        char *tmp = udisks_drive_dup_sibling_id(driveHandler);
        return charToQString(tmp);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "not valid property for this device");
        return "";
    }
}

QVariant DFMBlockDevicePrivate::getFileSystemProperty(Property name) const
{
    if (!fileSystemHandler) {
        return QStringList();
    }

    switch (name) {
    case Property::FileSystemMountPoint: {
        char **ret = udisks_filesystem_dup_mount_points(fileSystemHandler);
        return charToQStringList(ret);
    }
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
    }
}

QVariant DFMBlockDevicePrivate::getPartitionProperty(Property name) const
{
    if (!partitionHandler) {
        qWarning() << "this device do not have a partition";
        return "";
    }

    switch (name) {
    case Property::PartitionNumber:
        return uint(udisks_partition_get_number(partitionHandler));
    case Property::PartitionType: {
        char *tmp = udisks_partition_dup_type_(partitionHandler);
        return charToQString(tmp);
    }
    case Property::PartitionOffset:
        return quint64(udisks_partition_get_offset(partitionHandler));
    case Property::PartitionSize:
        return quint64(udisks_partition_get_size(partitionHandler));
    case Property::PartitionFlags:
        return quint64(udisks_partition_get_flags(partitionHandler));
    case Property::PartitionName: {
        char *tmp = udisks_partition_dup_uuid(partitionHandler);
        return charToQString(tmp);
    }
    case Property::PartitionUUID: {
        char *tmp = udisks_partition_dup_type_(partitionHandler);
        return charToQString(tmp);
    }
    case Property::PartitionTable: {
        char *tmp = udisks_partition_dup_table(partitionHandler);
        return charToQString(tmp);
    }
    case Property::PartitionIsContainer:
        return bool(udisks_partition_get_is_container(partitionHandler));
    case Property::PartitionIsContained:
        return bool(udisks_partition_get_is_contained(partitionHandler));
    default:
        Q_ASSERT_X(0, __FUNCTION__, "the property is not supported for block device");
    }
}

void DFMBlockDevicePrivate::init(UDisksClient *cli)
{
    Q_ASSERT_X(cli, __FUNCTION__, "client cannot be null");

    std::string str = blkObjPath.toStdString();
    UDisksObject *blkObj = udisks_client_peek_object(cli, str.c_str());
    if (!blkObj)
        return;
    blockHandler = udisks_object_peek_block(blkObj);
    fileSystemHandler = udisks_object_peek_filesystem(blkObj);
    partitionHandler = udisks_object_peek_partition(blkObj);
    encryptedHandler = udisks_object_peek_encrypted(blkObj);
    partitionTabHandler = udisks_object_peek_partition_table(blkObj);
    loopHandler = udisks_object_peek_loop(blkObj);

    if (blockHandler) {
        char *drvObjPath = udisks_block_dup_drive(blockHandler);
        UDisksObject *driveObj = udisks_client_peek_object(cli, drvObjPath);
        if (driveObj)
            driveHandler = udisks_object_peek_drive(driveObj);
    }
}
