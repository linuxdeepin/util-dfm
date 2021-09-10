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

#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include <functional>
#include <udisks/udisks.h>

DFM_MOUNT_USE_NS

DFMBlockDevice::DFMBlockDevice(QObject *parent)
    : DFMDevice(new DFMBlockDevicePrivate(this), parent)
{
    auto subd = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    registerPath(std::bind(&DFMBlockDevicePrivate::path, subd));
    registerMount(std::bind(&DFMBlockDevicePrivate::mount, subd, std::placeholders::_1));
    registerMountAsync(std::bind(&DFMBlockDevicePrivate::mountAsync, subd, std::placeholders::_1));
    registerUnmount(std::bind(&DFMBlockDevicePrivate::unmount, subd));
    registerUnmountAsync(std::bind(&DFMBlockDevicePrivate::unmountAsync, subd));
    registerRename(std::bind(&DFMBlockDevicePrivate::rename, subd, std::placeholders::_1));
    registerRenameAsync(std::bind(&DFMBlockDevicePrivate::renameAsync, subd, std::placeholders::_1));
    registerAccessPoint(std::bind(&DFMBlockDevicePrivate::accessPoint, subd));
    registerMountPoint(std::bind(&DFMBlockDevicePrivate::mountPoint, subd));
    registerFileSystem(std::bind(&DFMBlockDevicePrivate::fileSystem, subd));
    registerSizeTotal(std::bind(&DFMBlockDevicePrivate::sizeTotal, subd));
    registerSizeUsage(std::bind(&DFMBlockDevicePrivate::sizeUsage, subd));
    registerSizeFree(std::bind(&DFMBlockDevicePrivate::sizeFree, subd));
    registerDeviceType(std::bind(&DFMBlockDevicePrivate::deviceType, subd));
    registerGetProperty(std::bind(&DFMBlockDevicePrivate::getProperty, subd, std::placeholders::_1));
}

DFMBlockDevice::~DFMBlockDevice()
{
    qDebug() << __FUNCTION__ << "is released";
}

QString DFMBlockDevice::deviceDescriptor() const
{
    auto subD = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (subD)
        return subD->devDesc;
    return "";
}

bool DFMBlockDevice::eject()
{
    auto subD = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (subD)
        return subD->eject();
    return false;
}

bool DFMBlockDevice::powerOff()
{
    auto subD = castSubPrivate<DFMDevicePrivate, DFMBlockDevicePrivate>(d.data());
    if (subD)
        return subD->powerOff();
    return false;
}

DFMBlockDevicePrivate::DFMBlockDevicePrivate(DFMBlockDevice *qq)
    : DFMDevicePrivate(qq)
{

}

QString DFMBlockDevicePrivate::path() const
{
    return devDesc;
}

QUrl DFMBlockDevicePrivate::mount(const QVariantMap &opts)
{
    if (!fileSystemHandler) {
        lastError = MountError::ErrNotMountable;
        qWarning() << "device is not mountable";
        return QUrl();
    }

    QStringList mpts = getProperty(Property::FileSystemMountPoint).toStringList();
    if (!mpts.empty()) {
        lastError = MountError::ErrAlreadyMounted;
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
    QtConcurrent::run([&]{
        auto ret = mount(opts);
        Q_EMIT q->mounted(ret);
    });
}

bool DFMBlockDevicePrivate::unmount()
{
    return false;
}

void DFMBlockDevicePrivate::unmountAsync()
{
    QtConcurrent::run([&]{
        auto ret = unmount();
        if (ret)
            Q_EMIT q->unmounted();
    });
}

bool DFMBlockDevicePrivate::rename(const QString &newName)
{
    return false;
}

void DFMBlockDevicePrivate::renameAsync(const QString &newName)
{
    QtConcurrent::run([&]{
        auto ret = rename(newName);
        if (ret)
            Q_EMIT q->renamed(newName);
    });
}

QUrl DFMBlockDevicePrivate::accessPoint() const
{
    return QUrl();
}

QUrl DFMBlockDevicePrivate::mountPoint() const
{
    return QUrl();
}

QString DFMBlockDevicePrivate::fileSystem() const
{
    return getProperty(Property::BlockIDType).toString();
}

long DFMBlockDevicePrivate::sizeTotal() const
{
    return 0;
}

long DFMBlockDevicePrivate::sizeUsage() const
{
    return 0;
}

long DFMBlockDevicePrivate::sizeFree() const
{
    return 0;
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
    return false;
}

bool DFMBlockDevicePrivate::powerOff()
{
    return false;
}

// only for parse the return value of *_DUP_*, do not use it with *_GET_*
QString DFMBlockDevicePrivate::charToQString(char *tmp) const
{
    if (!tmp)
        return QString();
    QString ret(tmp);
    g_free(tmp);
    return ret;
}

// only for parse the return value of *_DUP_*, do not use it with *_GET_*
QStringList DFMBlockDevicePrivate::charToQStringList(char **tmp) const
{
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
