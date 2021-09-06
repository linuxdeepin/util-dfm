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
#ifndef DFMMOUNT_GLOBAL_H
#define DFMMOUNT_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DFMMOUNT_LIBRARY)
#  define DFMMOUNTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define DFMMOUNTSHARED_EXPORT Q_DECL_IMPORT
#endif

#define DFM_MOUNT_NAMESPACE dfm_mount
#define DFM_MOUNT_BEGIN_NS namespace DFM_MOUNT_NAMESPACE {
#define DFM_MOUNT_END_NS }
#define DFM_MOUNT_USE_NS using namespace DFM_MOUNT_NAMESPACE;

#define DFM_MNT_VIRTUAL
#define DFM_MNT_OVERRIDE

DFM_MOUNT_BEGIN_NS

enum class DeviceType : uint16_t {
    AllDevice      = 0,
    BlockDevice    = 1,    // normal block devices, like removable disks
    ProtocolDevice = 2,    // protocol devices, like smb/mtp/ftp/ptp etc...
    NetDevice      = 3,
};

enum class MonitorStatus : uint16_t {
    Monitoring  = 0,
    Idle        = 1,

    NotDefined  = 0xff,
};

enum class Property : uint16_t {
    // blocks property
    BlockProperty = 0,         // this is invalid but just a placeholder
    BlockConfiguration,        // variant
    BlockCryptoBackingDevice,  // str
    BlockDevice,
    BlockDrive,
    BlockIDLabel,
    BlockIDType,
    BlockIDUsage,              // uint64
    BlockIDUUID,               // str
    BlockIDVersion,
    BlockDeviceNumber,         // uint64
    BlockPreferredDevice,      // str
    BlockID,
    BlockSize,                 // uint64
    BlockReadOnly,             // bool
    BlockSymlinks,             // strlist
    BlockHintPartitionable,    // bool
    BlockHintSystem,
    BlockHintIgnore,
    BlockHintAuto,
    BlockHintName,             // str
    BlockHintIconName,
    BlockHintSymbolicIconName,
    BlockMdRaid,
    BlockMdRaidMember,
    BlockPropertyEND,

    DriveProperty = 30,
    DriveConnectionBus,        // str
    DriveRemovable,            // bool
    DriveEjectable,
    DriveSeat,                 // str
    DriveMedia,
    DriveMediaCompatibility,   // strlist
    DriveMediaRemovable,       // bool
    DriveMediaAvailable,
    DriveMediaChangeDetected,
    DriveTimeDetected,         // uint64
    DriveTimeMediaDetected,
    DriveSize,
    DriveOptical,              // bool
    DriveOpticalBlank,
    DriveOpticalNumTracks,     // uint
    DriveOpticalNumAudioTracks,
    DriveOpticalNumDataTracks,
    DriveOpticalNumSessions,
    DriveModel,                // str
    DriveRevision,
    DriveRotationRate,         // int
    DriveSerial,               // str
    DriveVender,
    DriveWWN,
    DriveSortKey,
    DriveConfiguration,        // variant
    DriveID,                   // str
    DriveCanPowerOff,
    DriveSiblingID,
    DrivePropertyEND,

    FileSystemProperty,
    FileSystemMountPoint,      // strlist
    FileSystemPropertyEND,

    PartitionProperty,
    PartitionNumber,           // uint
    PartitionType,             // str
    PartitionOffset,           // uint64
    PartitionSize,
    PartitionFlags,
    PartitionName,             // str
    PartitionUUID,
    PartitionTable,
    PartitionIsContainer,      // bool
    PartitionIsContained,
    PartitionPropertyEND,

    // protocol property
    ProtoProperty = 800,       // this is invalid but just a placeholder
};

enum class MountError : uint16_t {
    ErrNotMountable = 0,    // which means there is no filesystem interface for block devices.
    ErrAlreadyMounted,
};

DFM_MOUNT_END_NS
#endif // DFMMOUNT_GLOBAL_H
