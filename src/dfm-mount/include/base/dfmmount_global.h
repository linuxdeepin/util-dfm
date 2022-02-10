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
#    define DFMMOUNTSHARED_EXPORT Q_DECL_EXPORT
#else
#    define DFMMOUNTSHARED_EXPORT Q_DECL_IMPORT
#endif

#define DFMMOUNT dfmmount
#define DFM_MOUNT_BEGIN_NS namespace DFMMOUNT {
#define DFM_MOUNT_END_NS }
#define DFM_MOUNT_USE_NS using namespace DFMMOUNT;

#define DFM_MNT_VIRTUAL
#define DFM_MNT_OVERRIDE

DFM_MOUNT_BEGIN_NS

enum class DeviceType : uint16_t {
    AllDevice = 0,
    BlockDevice = 1,   // normal block devices, like removable disks
    ProtocolDevice = 2,   // protocol devices, like smb/mtp/ftp/ptp etc...
    NetDevice = 3,
};

enum class NetworkMountPasswdSaveMode : uint8_t {
    NeverSavePasswd = 0,
    SaveBeforeLogout,
    SavePermanently,
};

enum class MonitorStatus : uint16_t {
    Monitoring = 0,
    Idle = 1,

    NotDefined = 0xff,
};

enum class Property : uint16_t {
    NotInit = 0,
    // blocks property
    BlockProperty,   // this is invalid but just a placeholder
    BlockConfiguration,   // variant
    BlockCryptoBackingDevice,   // str
    BlockDevice,
    BlockDrive,
    BlockIDLabel,
    BlockIDType,
    BlockIDUsage,   // uint64
    BlockIDUUID,   // str
    BlockIDVersion,
    BlockDeviceNumber,   // uint64
    BlockPreferredDevice,   // str
    BlockID,
    BlockSize,   // uint64
    BlockReadOnly,   // bool
    BlockSymlinks,   // strlist
    BlockHintPartitionable,   // bool
    BlockHintSystem,
    BlockHintIgnore,
    BlockHintAuto,
    BlockHintName,   // str
    BlockHintIconName,
    BlockHintSymbolicIconName,
    BlockMdRaid,
    BlockMdRaidMember,
    BlockPropertyEND,

    DriveProperty = 30,
    DriveConnectionBus,   // str
    DriveRemovable,   // bool
    DriveEjectable,
    DriveSeat,   // str
    DriveMedia,
    DriveMediaCompatibility,   // strlist
    DriveMediaRemovable,   // bool
    DriveMediaAvailable,
    DriveMediaChangeDetected,
    DriveTimeDetected,   // uint64
    DriveTimeMediaDetected,
    DriveSize,
    DriveOptical,   // bool
    DriveOpticalBlank,
    DriveOpticalNumTracks,   // uint
    DriveOpticalNumAudioTracks,
    DriveOpticalNumDataTracks,
    DriveOpticalNumSessions,
    DriveModel,   // str
    DriveRevision,
    DriveRotationRate,   // int
    DriveSerial,   // str
    DriveVender,
    DriveWWN,
    DriveSortKey,
    DriveConfiguration,   // variant
    DriveID,   // str
    DriveCanPowerOff,   // bool
    DriveSiblingID,   // str
    DrivePropertyEND,

    FileSystemProperty,
    FileSystemMountPoint,   // strlist
    FileSystemPropertyEND,

    PartitionProperty,
    PartitionNumber,   // uint
    PartitionType,   // str
    PartitionOffset,   // uint64
    PartitionSize,
    PartitionFlags,
    PartitionName,   // str
    PartitionUUID,
    PartitionTable,
    PartitionIsContainer,   // bool
    PartitionIsContained,
    PartitionPropertyEND,

    EncryptedProperty,
    EncryptedChildConfiguration,   // variant
    EncryptedCleartextDevice,   // str
    EncryptedHintEncryptionType,
    EncryptedMetadataSize,   // uint64
    EncryptedPropertyEnd,

    // protocol property
    ProtoProperty = 800,   // this is invalid but just a placeholder
};

#define UDISKS_ERR_DOMAIN "udisks-error-quark"
#define UDISKS_ERR_START 0
#define GIO_ERR_DOMAIN "g-io-error-quark"
#define GIO_ERR_START 200
#define GDBUS_ERR_DOMAIN "g-dbus-error-quark"
#define GDBUS_ERR_START 400
#define USER_ERR_START 800
enum class DeviceError : uint16_t {
    NoError = 10000,

    // ￬ these errors are transfered from udisks
    UDisksErrorFailed = UDISKS_ERR_START,
    UDisksErrorCancelled,
    UDisksErrorAlreadyCancelled,
    UDisksErrorNotAuthorized,
    UDisksErrorNotAuthorizedCanObtain,
    UDisksErrorNotAuthorizedDismissed,
    UDisksErrorAlreadyMounted,
    UDisksErrorNotMounted,
    UDisksErrorOptionNotPermitted,
    UDisksErrorMountedByOtherUser,
    UDisksErrorAlreadyUnmounting,
    UDisksErrorNotSupproted,
    UDisksErrorTimedOut,
    UDisksErrorWouldWakeup,
    UDisksErrorDeviceBusy,
    UDisksErrorScsiDaemonTransportFailed,
    UDisksErrorScsiHostNotFound,
    UDisksErrorScsiIDMB,
    UDisksErrorScsiLoginFailed,
    UDisksErrorScsiLoginAuthFailed,
    UDisksErrorScsiLoginFatal,
    UDisksErrorScsiLogoutFailed,
    UDisksErrorScsiNoFirmware,
    UDisksErrorScsiNoObjectsFound,
    UDisksErrorScsiNotConnected,
    UDisksErrorScsiTransportFailed,
    UDisksErrorScsiUnknownDiscoveryType,   // ￪ these errors are transfered from udisks

    // these errors are from GIOErrorEnum
    GIOError = GIO_ERR_START,
    GIOErrorFailed = GIOError,
    GIOErrorNotFound,
    GIOErrorExists,
    GIOErrorIsDirectory,
    GIOErrorNotDirertory,
    GIOErrorNotEmpty,
    GIOErrorNotRegularFile,
    GIOErrorNotSymbolicLink,
    GIOErrorNotMountableFile,
    GIOErrorFilenameTooLong,
    GIOErrorInvalidFilename,
    GIOErrorTooManyLinks,
    GIOErrorNoSpace,
    GIOErrorInvalidArgument,
    GIOErrorPermissionDenied,
    GIOErrorNotSupported,
    GIOErrorNotMounted,
    GIOErrorAlreadyMounted,
    GIOErrorClosed,
    GIOErrorCancelled,
    GIOErrorPending,
    GIOErrorReadONly,
    GIOErrorCantCreateBackup,
    GIOErrorWrongETAG,
    GIOErrorTimedOut,
    GIOErrorWouldRecurse,
    GIOErrorBusy,
    GIOErrorWouldBlock,
    GIOErrorHostNotFound,
    GIOErrorWouldMerge,
    GIOErrorFailedHandled,
    GIOErrorTooManyOpenFiles,
    GIOErrorNotInitilized,
    GIOErrorAddressInUse,
    GIOErrorPartialInput,
    GIOErrorInvalidData,
    GIOErrorDBusError,
    GIOErrorHostUnreachable,
    GIOErrorNetworkUnreachable,
    GIOErrorConnectionRefused,
    GIOErrorProxyFailed,
    GIOErrorProxyAuthFailed,
    GIOErrorProxyNeedAuth,
    GIOErrorProxyNotAllowed,
    GIOErrorBrokenPipe,
    GIOErrorConnectionClosed = GIOErrorBrokenPipe,
    GIOErrorNotConnected,
    GIOErrorMessageTooLarge,

    // these are from GDBusError
    GDBusError = GDBUS_ERR_START,
    GDBusErrorFailed = GDBusError,
    GDBusErrorNoMemory,
    GDBusErrorServiceUnknown,
    GDBusErrorNameHasNoOwner,
    GDBusErrorNoReply,
    GDBusErrorIOError,
    GDBusErrorBadAddress,
    GDBusErrorNotSupported,
    GDBusErrorLimitsExceeded,
    GDBusErrorAccessDenied,
    GDBusErrorAuthFailed,
    GDBusErrorNoServer,
    GDBusErrorTimeout,
    GDBusErrorNoNetwork,
    GDBusErrorAddressInUse,
    GDBusErrorDisconnected,
    GDBusErrorInvalidArgs,
    GDBusErrorFileNotFound,
    GDBusErrorFileExists,
    GDBusErrorUnknownMethod,
    GDBusErrorTimedOut,
    GDBusErrorMatchRuleNotFound,
    GDBusErrorMatchRuleInvalid,
    GDBusErrorSpawnExecFailed,
    GDBusErrorSpawnForkFailed,
    GDBusErrorSpawnChildExited,
    GDBusErrorSpawnChildSignaled,
    GDBusErrorSpawnFailed,
    GDBusErrorSpawnSetupFailed,
    GDBusErrorSpawnConfigInvalid,
    GDBusErrorSpawnServiceInvalid,
    GDBusErrorSpawnServiceNotFound,
    GDBusErrorSpawnPermissionsInvalid,
    GDBusErrorSpawnFileInvalid,
    GDBusErrorSpawnNoMemory,
    GDBusErrorUnixProcessIdUnkown,
    GDBusErrorInvalidSignature,
    GDBusErrorInvalidFileContent,
    GDBusErrorSeLinuxSecurityContextUnknown,
    GDBusErrorADTAuditDataUnknown,
    GDBusErrorObjectPathInUse,
    GDBusErrorUnknownObject,
    GDBusErrorUnknownInterface,
    GDBusErrorUnknownProperty,
    GDBusErrorPropertyReadOnly,

    UserError = USER_ERR_START,
    UserErrorNotMountable,   // ￬ these are by ourselves
    UserErrorNotEjectable,
    UserErrorNoDriver,
    UserErrorNotEncryptable,
    UserErrorNoPartition,
    UserErrorNoBlock,
    UserErrorNetworkWrongPasswd,
    UserErrorNetworkAnonymousNotAllowed,
    UserErrorUserCancelled,
    UserErrorTimedOut,
    UserErrorAlreadyMounted,
    UserErrorNotMounted,
    UserErrorFailed,

    UnhandledError = 1000,
};

enum class MonitorError : uint16_t {
    NoError = 0,
    MonitorAlreadyRegistered,
    MonitorNotRegister,
};

enum class PartitionType : uint16_t {
    PartitionTypeNotFound = 65535,

    MbrEmpty = 0x00,
    MbrFAT12Type = 0x01,
    MbrXENIX_root = 0x02,
    MbrXENIX_usr = 0x03,
    MbrFAT16_Less_32M = 0x04,
    MbrExtended = 0x05,
    MbrFAT16Type = 0x06,
    MbrHPFS_NTFS = 0x07,
    MbrAIX = 0x08,
    MbrAIX_bootable = 0x09,
    MbrOS2_Boot_Manager = 0x0a,
    MbrWin95_FAT32 = 0x0b,
    MbrWin95_FAT32_LBA = 0x0c,
    MbrWin95_FAT16_LBA = 0x0e,
    MbrWin95_Extended_LBA = 0x0f,
    MbrOPUS = 0x10,
    MbrHidden_FAT12 = 0x11,
    MbrCompaq_diagnostics = 0x12,
    MbrHidden_FAT16_Less_32M = 0x14,
    MbrHidden_FAT16 = 0x16,
    MbrHidden_HPFS_or_NTFS = 0x17,
    MbrAST_SmartSleep = 0x18,
    MbrHidden_Win95_FAT32 = 0x1b,
    MbrHidden_Win95_FAT32_LBA = 0x1c,
    MbrHidden_Win95_FAT16_LBA = 0x1e,
    MbrNEC_DOS = 0x24,
    MbrPlan9 = 0x39,
    MbrPartitionMagic_recovery = 0x3c,
    MbrVenix_80286 = 0x40,
    MbrPPC_PReP_Boot = 0x41,
    MbrSFS = 0x42,
    MbrQNX4_dot_x = 0x4d,
    MbrQNX4_dot_x_2nd_part = 0x4e,
    MbrQNX4_dot_x_3rd_part = 0x4f,
    MbrOnTrack_DM = 0x50,
    MbrOnTrack_DM6_Aux1 = 0x51,
    MbrCP_M = 0x52,
    MbrOnTrack_DM6_Aux3 = 0x53,
    MbrOnTrackDM6 = 0x54,
    MbrEZ_Drive = 0x55,
    MbrGolden_Bow = 0x56,
    MbrPriam_Edisk = 0x5c,
    MbrSpeedStor = 0x61,
    MbrGNU_HURD_or_SysV = 0x63,
    MbrNovell_Netware_286 = 0x64,
    MbrNovell_Netware_386 = 0x65,
    MbrDiskSecure_Multi_Boot = 0x70,
    MbrPC_IX = 0x75,
    MbrOld_Minix = 0x80,
    MbrMinix_old_Linux = 0x81,
    MbrLinux_swap = 0x82,
    MbrLinux = 0x83,
    MbrOS2_hidden_C_drive = 0x84,
    MbrLinux_extended = 0x85,
    MbrNTFS_volume_set_1 = 0x86,
    MbrNTFS_volume_set_2 = 0x87,
    MbrLinux_LVM = 0x8e,
    MbrAmoeba = 0x93,
    MbrAmoeba_BBT = 0x94,
    MbrBSD_OS = 0x9f,
    MbrIBM_Thinkpad_hibernation = 0xa0,
    MbrFreeBSD = 0xa5,
    MbrOpenBSD = 0xa6,
    MbrNeXTSTEP = 0xa7,
    MbrNetBSD = 0xa8,
    MbrBSDI_fs = 0xa9,
    MbrBSDI_swap = 0xb7,
    MbrBoot_Wizard_hidden = 0xb8,
    MbrDRDOS_sec_FAT12 = 0xbb,
    MbrDRDOS_sec_FAT16_Less_32M = 0xc1,
    MbrDRDOS_sec_FAT16 = 0xc4,
    MbrDRDOS_sec_extend = 0xc5,
    MbrSyrinx = 0xc6,
    MbrNon_FS_data = 0xc7,
    MbrMultiuser_DOS_extend = 0xd5,
    MbrCP_M_CTOS_dot_dot_dot = 0xda,
    MbrDell_Utility = 0xdb,
    MbrBootIt = 0xde,
    MbrDOS_access = 0xdf,
    MbrDOS_R_O = 0xe1,
    MbrSpeedStor_1 = 0xe3,
    MbrBeOS_fs = 0xe4,
    MbrEFI_GPT = 0xeb,
    MbrEFI_FAT12_16_32 = 0xee,
    MbrLinux_PA_RISC_boot = 0xef,
    MbrSpeedStor_2 = 0xf0,
    MbrSeppdStor_3 = 0xf4,
    MbrDOS_secondary = 0xf2,
    MbrLinux_raid_autodetect = 0xfd,
    MbrLANstep = 0xfe,
    MbrBBT = 0xff,

    // Gpt[Name][FileSystem]
    // https://en.wikipedia.org/wiki/GUID_Partition_Table
    GptUnusedEntryNA,
    GptMBRPartitionSchemeNA,
    GptEFISystemPartitionNA,
    GptBIOSBootPartitionNA,
    GptIntelFastFlashPartitionNA,
    GptSonyBootPartitionNA,
    GptLenovoBootPartitionNA,

    GptMicrosoftReservedPartitionWin,
    GptBasicDataPartitionWin,
    GptLogicalDiskManagerMetaDataPartitionWin,
    GptLogicalDiskManagerDataPartitionWin,
    GptWindowsRecoveryEnvironmentWin,
    GptIBMGeneralParallelFileSystemPartitionWin,
    GptStorageSpacesPartitionWin,
    GptStorageReplicaPartitionWin,

    GptDataPartitionHPUX,
    GptServicePartitionHPUX,

    GptLinuxFilesystemDataLinux,
    GptRAIDPartitionLinux,
    GptRootPartitionX86Linux,
    GptRootPartitionX8664Linux,
    GptRootPartitionArm32Linux,
    GptRootPartitionArm64Linux,
    GptBootPartitionLinux,
    GptSwapPartitionLinux,
    GptLogicalVolumeManagerPartitionLinux,
    GptHomePartitionLinux,
    GptServerDataPartitionLinux,
    GptPlainDMCryptPartitionLinux,
    GptLUKSPartitionLinux,
    GptReservedLinux,

    GptBootPartitionFreeBSD,
    GptBSDDisklabelPartitionFreeBSD,
    GptSwapPartitionFreeBSD,
    GptUnixFileSystemPartitionFreeBSD,
    GptVinumVolumeManagerPartitionFreeBSD,
    GptZFSPartitionFreeBSD,
    GptNandfsPartitionFreeBSD,

    GptHierarchialFileSystemPlusPartitionMacOS,
    GptAppleAPFSContainerMacOS,
    GptAppleUFSContainerMacOS,
    GptZFSMacOS,
    GptAppleRAIDPartitionMacOS,
    GptAppleRAIDPartitionOfflineMacOS,
    GptAppleBootPartitionMacOS,
    GptAppleLabelMacOS,
    GptAppleTVRecoveryPartitionMacOS,
    GptAppleCoreStorageContainerMacOS,
    GptAppleAPFSPrebootPartitionMacOS,
    GptAppleAPFSRecoveryPartitionMacOS,

    GptBootPartitionSolaris,
    GptRootPartitionSolaris,
    GptSwapPartitionSolaris,
    GptBackupPartitionSolaris,
    GptUsrPartitionSolaris,
    GptVarPartitionSolaris,
    GptHomePartitionSolaris,
    GptAlternateSectorSolaris,
    GptReservedPartitionSolaris,

    GptSwapPartitionNetBSD,
    GptFFSPartitionNetBSD,
    GptLFSPartitionNetBSD,
    GptRAIDPartitionNetBSD,
    GptConcatenatedPartitionNetBSD,
    GptEncryptedPartitionNetBSD,

    GptKernelChromeOS,
    GptRootfsChromeOS,
    GptFirmwareChromeOS,
    GptFutureUseChromeOS,
    GptMiniOSChromeOS,
    GptHibernateChromeOS,

    GptUsrPartitionCoreOS,
    GptResizableRootfsCoreOS,
    GptOEMCustomizationsCoreOS,
    GptRootFilesystemOnRAIDCoreOS,

    GptHaikuBFSHaiku,

    GptBootPartitionMidnightBSD,
    GptDataPartitionMidnightBSD,
    GptSwapPartitionMidnightBSD,
    GptUnixFileSystemPartitionMidnightBSD,
    GptVinumVolumemanagerPartitionMidnightBSD,
    GptZFSPartitionMidnightBSD,

    GptJournalCeph,
    GptDmCryptJournalCeph,
    GptOSDCeph,
    GptDmCryptOSDCeph,
    GptDiskinCreationCeph,
    GptDmCryptDiskinCreationCeph,
    GptBlockCeph,
    GptBlockDBCeph,
    GptBlockWriteAheadlogCeph,
    GptLockboxForDmCryptKeysCeph,
    GptMultipathOSDCeph,
    GptMultipathJournalCeph,
    GptMultipathBlockCeph,
    GptMultipathBlockDBCeph,
    GptMultipathblockwriteAheadogCeph,
    GptDmCryptBlockCeph,
    GptDmCryptBlockDBCeph,
    GptDmCryptBlockWriteAheadlogCeph,
    GptDmCryptLUKSjournalCeph,
    GptDmCryptLUKSBlockCeph,
    GptDmCryptLUKSBlockDBCeph,
    GptDmCryptLUKSBlockwriteAheadlogCeph,
    GptDmCryptLUKSOSDCeph,

    GptDataPartitionOpenBSD,

    GptPowerSafeFilesystemQNX,

    GptPlan9PartitionPlan9,

    GptVmkCoreVMwareESX,
    GptVMFSFilesystemPartitionVMwareESX,
    GptVMWareReservedVMwareESX,

    GptBootloaderAndroidIA,
    GptBootloader2AndroidIA,
    GptBootAndroidIA,
    GptRecoveryAndroidIA,
    GptMiscAndroidIA,
    GptMetadataAndroidIA,
    GptSystemAndroidIA,
    GptCacheAndroidIA,
    GptDataAndroidIA,
    GptPersistentAndroidIA,
    GptVendorAndroidIA,
    GptConfigAndroidIA,
    GptFactoryAndroidIA,
    GptFactoryAltAndroidIA,
    GptFastbootOrTertiaryAndroidIA,
    GptOEMAndroidIA,

    GptAndroidMetaAndroid6Arm,
    GptAndroidEXTAndroid6Arm,

    GptBootONIE,
    GptConfigONIE,

    GptPRePBootPowerPC,

    GptSharedBootloaderConfigurationFreedesktop,

    GptBasicDataPartitionAtariTOS,

    GptEncryptedDataPartitionVeraCrypt,

    GptArcaOSType1OS2,

    GptSPDKBlockDeviceSPDK,

    GptBareBoxStateBareboxBootloader,

    GptUBootEnvironmentUBootBootloader,

    GptStatusSoftRAID,
    GptScratchSoftRAID,
    GptVolumeSoftRAID,
    GptCacheSoftRAID,

    GptBootloaderFuchsiaStandard,
    GptDurablemutableencryptedsystemdataFuchsiaStandard,
    GptDurablemutablebootloaderdataFuchsiaStandard,
    GptFactoryProvisionedreadOnlysystemdataFuchsiaStandard,
    GptFactoryProvisionedreadOnlybootloaderdataFuchsiaStandard,
    GptFuchsiaVolumeManagerFuchsiaStandard,
    GptVerifiedbootmetadataFuchsiaStandard,
    GptZirconbootimageFuchsiaStandard,

    GptFuchsiaEspFuchsiaLegacy,
    GptFuchsiaSystemFuchsiaLegacy,
    GptFuchsiaDataFuchsiaLegacy,
    GptFuchsiaInstallFuchsiaLegacy,
    GptFuchsiaBlobFuchsiaLegacy,
    GptFuchsiaFvmFuchsiaLegacy,
    GptZirconbootimageSlotAFuchsiaLegacy,
    GptZirconbootimageSlotBFuchsiaLegacy,
    GptZirconbootimageSlotRFuchsiaLegacy,
    GptSysConfigFuchsiaLegacy,
    GptFactoryConfigFuchsiaLegacy,
    GptBootloaderFuchsiaLegacy,
    GptGuidTestFuchsiaLegacy,
    GptVerifiedbootmetadataSlotAFuchsiaLegacy,
    GptVerifiedbootmetadataSlotBFuchsiaLegacy,
    GptVerifiedbootmetadataSlotRFuchsiaLegacy,
    GptMiscFuchsiaLegacy,
    GptEmmcBoot1FuchsiaLegacy,
    GptEmmcBoot2FuchsiaLegacy,
};

DFM_MOUNT_END_NS
#endif   // DFMMOUNT_GLOBAL_H
