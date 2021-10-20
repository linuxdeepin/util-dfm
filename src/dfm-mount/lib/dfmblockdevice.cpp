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

#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QStorageInfo>

#include <functional>
#include <udisks/udisks.h>

DFM_MOUNT_USE_NS

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

static GAsyncReadyCallback mountAsyncCallback = [](GObject *source_object, GAsyncResult *res, gpointer user_data) {
    DFMBlockDevice *dev = static_cast<DFMBlockDevice *>(user_data);
    Q_ASSERT_X(dev, __FUNCTION__, "device is not valid");

    char *mountPoint = nullptr;
    GError *err = nullptr;

    UDisksFilesystem *fs = UDISKS_FILESYSTEM(source_object);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");

    bool result = udisks_filesystem_call_mount_finish(fs, &mountPoint, res, &err);
    if (!result)
        ; // TODO: handle error

    if (mountPoint)
        g_free(mountPoint);
    if (err)
        g_error_free(err);
};

static GAsyncReadyCallback unmountAsyncCallback = [](GObject *source_object, GAsyncResult *res, gpointer user_data) {
    DFMBlockDevice *dev = static_cast<DFMBlockDevice *>(user_data);
    Q_ASSERT_X(dev, __FUNCTION__, "device is not valid");

    UDisksFilesystem *fs = UDISKS_FILESYSTEM(source_object);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");

    GError *err = nullptr;
    bool result = udisks_filesystem_call_unmount_finish(fs, res, &err);
    if (!result)
        // TODO: handle the errors

    if (err)
        g_error_free(err);
};

static GAsyncReadyCallback renameAsyncCallback = [](GObject *source_object, GAsyncResult *res, gpointer user_data) {
    DFMBlockDevice *dev = static_cast<DFMBlockDevice *>(user_data);
    Q_ASSERT_X(dev, __FUNCTION__, "device is not valid");

    UDisksFilesystem *fs = UDISKS_FILESYSTEM(source_object);
    Q_ASSERT_X(fs, __FUNCTION__, "fs is not valid");

    GError *err = nullptr;
    bool result = udisks_filesystem_call_set_label_finish(fs, res, &err);
    if (result) {
        QString newLabel = dev->getProperty(Property::BlockIDLabel).toString();
        Q_EMIT dev->renamed(newLabel);
    } else {
        // TODO: error handle
        if (err)
            g_error_free(err);
    }
};

static GAsyncReadyCallback ejectAsyncCallback = [](GObject *source_object, GAsyncResult *res, gpointer user_data) {
    DFMBlockDevice *dev = static_cast<DFMBlockDevice *>(user_data);
    Q_ASSERT_X(dev, __FUNCTION__, "device is not valid");

    UDisksDrive *drive = UDISKS_DRIVE(source_object);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");

    GError *err = nullptr;
    bool result = udisks_drive_call_eject_finish(drive, res, &err);
    if (result) {
        Q_EMIT dev->ejected();
    } else {
        // TODO: handle errors
        g_error_free(err);
    }
};

static GAsyncReadyCallback powerOffAsyncCallback = [](GObject *source_object, GAsyncResult *res, gpointer user_data) {
    DFMBlockDevice *dev = static_cast<DFMBlockDevice *>(user_data);
    Q_ASSERT_X(dev, __FUNCTION__, "device is not valid");

    UDisksDrive *drive = UDISKS_DRIVE(source_object);
    Q_ASSERT_X(drive, __FUNCTION__, "drive is not valid");

    GError *err = nullptr;
    bool result = udisks_drive_call_power_off_finish(drive, res, &err);
    if (result) {
        Q_EMIT dev->powerOffed();
    } else {
        // TODO: handle errors
        g_error_free(err);
    }
};

DFMBlockDevice::DFMBlockDevice(QObject *parent)
    : DFMDevice(new DFMBlockDevicePrivate(this), parent)
{
    auto dp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    registerPath(std::bind(&DFMBlockDevicePrivate::path, dp));
    registerMount(std::bind(&DFMBlockDevicePrivate::mount, dp, std::placeholders::_1));
    registerMountAsync(std::bind(&DFMBlockDevicePrivate::mountAsync, dp, std::placeholders::_1));
    registerUnmount(std::bind(&DFMBlockDevicePrivate::unmount, dp));
    registerUnmountAsync(std::bind(&DFMBlockDevicePrivate::unmountAsync, dp));
    registerRename(std::bind(&DFMBlockDevicePrivate::rename, dp, std::placeholders::_1));
    registerRenameAsync(std::bind(&DFMBlockDevicePrivate::renameAsync, dp, std::placeholders::_1));
    registerAccessPoint(std::bind(&DFMBlockDevicePrivate::accessPoint, dp));
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
    qDebug() << __FUNCTION__ << "is released";
}

bool DFMBlockDevice::eject()
{
    auto dp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp)
        return dp->eject();
    return false;
}

void DFMBlockDevice::ejectAsync()
{
    auto dp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp)
        return dp->ejectAsync();
}

bool DFMBlockDevice::powerOff()
{
    auto dp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp)
        return dp->powerOff();
    return false;
}

void DFMBlockDevice::powerOffAsync()
{
    auto dp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (dp)
        return dp->powerOffAsync();
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
    auto dp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp->encryptedHandler != nullptr;
}

bool DFMBlockDevice::hasFileSystem() const
{
    auto dp = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    return dp->fileSystemHandler != nullptr;
}

bool DFMBlockDevice::hintIgnore() const
{
    return getProperty(Property::BlockHintIgnore).toBool();
}

DFMBlockDevicePrivate::DFMBlockDevicePrivate(DFMBlockDevice *qq)
    : DFMDevicePrivate(qq)
{

}

QString DFMBlockDevicePrivate::path() const
{
    return blkObjPath;
}

QUrl DFMBlockDevicePrivate::mount(const QVariantMap &opts)
{
    Q_UNUSED(opts); // TODO: use it later
    if (!fileSystemHandler) {
        lastError = MountError::NotMountable;
        qWarning() << "device is not mountable";
        return QUrl();
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = MountError::AlreadyMounted;
        qWarning() << "device is already mounted at " << mpts;
        return mpts.first();
    }

    GError *err = nullptr;
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);

    char *mountPoint = nullptr;
    bool mounted = udisks_filesystem_call_mount_sync(fileSystemHandler, gOpts, &mountPoint, nullptr, &err);

    QUrl ret;
    if (mounted && mountPoint) {
        // TODO: we need to complete the SCHEME later
        ret.setUrl(QString(mountPoint));
        g_free(mountPoint);
    }
    if (err)
        g_error_free(err);

    return ret;
}

void DFMBlockDevicePrivate::mountAsync(const QVariantMap &opts)
{
    Q_UNUSED(opts); // TODO: use it later

    if (!fileSystemHandler) {
        lastError = MountError::NotMountable;
        qWarning() << "device is not mountable";
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = MountError::AlreadyMounted;
        qWarning() << "device is already mounted at " << mpts;
        return;
    }

    // mount device async
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);
    udisks_filesystem_call_mount(fileSystemHandler, gOpts, nullptr, mountAsyncCallback, q);
}

bool DFMBlockDevicePrivate::unmount()
{
    if (!fileSystemHandler) {
        lastError = MountError::NotMountable;
        qWarning() << "device is not mountable";
        return true; // since device is not mountable, then it cannot be mounted
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = MountError::NotMounted;
        qWarning() << "device is not mounted";
        return true; // since it's not mounted, then this invocation returns true
    }

    // start unmount device sync
    // construct the options
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);

    GError *err = nullptr;

    // unmount sync
    bool result = udisks_filesystem_call_unmount_sync(fileSystemHandler, gOpts, nullptr, &err);
    if (result) {
        if (err)
            g_error_free(err);
        Q_EMIT q->unmounted();
        return true;
    }

    // handle the errors
    if (err)
        g_error_free(err);
    return false;
}

void DFMBlockDevicePrivate::unmountAsync()
{
    if (!fileSystemHandler) {
        lastError = MountError::NotMountable;
        qWarning() << "device is not mountable";
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (mpts.empty()) {
        lastError = MountError::NotMounted;
        qWarning() << "device is not mounted";
        return;
    }

    // start unmount device async
    // construct the options
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);

    udisks_filesystem_call_unmount(fileSystemHandler, gOpts, nullptr, unmountAsyncCallback, q);
}

bool DFMBlockDevicePrivate::rename(const QString &newName)
{
    if (!fileSystemHandler) {
        lastError = MountError::NotMountable;
        qWarning() << "device is not mountable";
        return false;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = MountError::AlreadyMounted;
        qWarning() << "device is mounted, you have to unmount first";
        return false;
    }

    // construct the options
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);
    const char *label = newName.toStdString().c_str();
    GError *err = nullptr;

    bool result = udisks_filesystem_call_set_label_sync(fileSystemHandler, label, gOpts, nullptr, &err);
    if (result) {
        if (err)
            g_error_free(err);
        Q_EMIT q->renamed(newName);
        return true;
    }

    // TODO: handle error
    if (err)
        g_error_free(err);
    return false;
}

void DFMBlockDevicePrivate::renameAsync(const QString &newName)
{
    if (!fileSystemHandler) {
        lastError = MountError::NotMountable;
        qWarning() << "device is not mountable";
        return;
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = MountError::AlreadyMounted;
        qWarning() << "device is mounted, you have to unmount first";
        return;
    }

    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);
    const char *label = newName.toStdString().c_str();

    udisks_filesystem_call_set_label(fileSystemHandler, label, gOpts, nullptr, renameAsyncCallback, q);
}

QUrl DFMBlockDevicePrivate::accessPoint() const
{
    // TODO: only optical devices have a virtual url for now, so return the real path for normal devices
    // and the virtual path for optical devices, finish them later.
    return mountPoint();
}

QUrl DFMBlockDevicePrivate::mountPoint() const
{
    auto mpts = q->getProperty(Property::FileSystemMountPoint).toStringList();
    return mpts.isEmpty() ? QUrl() : QUrl(mpts.first());
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
        qWarning() << "this device is not mountable";
        return "";
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

bool DFMBlockDevicePrivate::eject()
{
    bool ejectable = q->getProperty(Property::DriveEjectable).toBool();
    if (!ejectable) {
        lastError = MountError::NotEjectable;
        qWarning() << "device is not ejectable";
        return false;
    }

    if (!driveHandler) {
        lastError = MountError::NoDriver;
        qWarning() << "device DO NOT have a driver, cannot eject";
        return false;
    }

    // construct the options
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);
    GError *err = nullptr;

    bool result = udisks_drive_call_eject_sync(driveHandler, gOpts, nullptr, &err);
    if (!result) {
        // TODO: handle the errors
        if (err)
            g_error_free(err);
        return false;
    }
    return true;
}

void DFMBlockDevicePrivate::ejectAsync()
{
    bool ejectable = q->getProperty(Property::DriveEjectable).toBool();
    if (!ejectable) {
        lastError = MountError::NotEjectable;
        qWarning() << "device is not ejectable";
        return;
    }

    if (!driveHandler) {
        lastError = MountError::NoDriver;
        qWarning() << "device DO NOT have a driver, cannot eject";
        return;
    }

    // construct the options
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);
    udisks_drive_call_eject(driveHandler, gOpts, nullptr, ejectAsyncCallback, q);
}

bool DFMBlockDevicePrivate::powerOff()
{
    if (!driveHandler) {
        lastError = MountError::NoDriver;
        qWarning() << "device DO NOT have a driver, cannot poweroff";
        return false;
    }

    // construct the options
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);

    GError *err = nullptr;
    bool result = udisks_drive_call_power_off_sync(driveHandler, gOpts, nullptr, &err);
    if (result) {
        return true;
    }

    // TODO: handle the errors
    if (err)
        g_error_free(err);
    return false;
}

void DFMBlockDevicePrivate::powerOffAsync()
{
    if (!driveHandler) {
        lastError = MountError::NoDriver;
        qWarning() << "device DO NOT have a driver, cannot poweroff";
        return;
    }

    // construct the options
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    // TODO: convert QVariantMap to GVariant and figure out whether builder and gvariant need to be released manully.
    GVariant *gOpts = g_variant_builder_end(builder);
    udisks_drive_call_power_off(driveHandler, gOpts, nullptr, powerOffAsyncCallback, q);
}
