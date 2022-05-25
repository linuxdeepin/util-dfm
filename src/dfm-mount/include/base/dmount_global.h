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
#ifndef DMOUNT_GLOBAL_H
#define DMOUNT_GLOBAL_H

#include <QtCore/qglobal.h>
#include <QtCore/qobjectdefs.h>

#include <functional>

#if defined(DMOUNT_LIBRARY)
#    define DMOUNTSHARED_EXPORT Q_DECL_EXPORT
#else
#    define DMOUNTSHARED_EXPORT Q_DECL_IMPORT
#endif

#define DFMMOUNT dfmmount
#define DFM_MOUNT_BEGIN_NS namespace DFMMOUNT {
#define DFM_MOUNT_END_NS }
#define DFM_MOUNT_USE_NS using namespace DFMMOUNT;

#define DMNT_VIRTUAL
#define DMNT_OVERRIDE

DFM_MOUNT_BEGIN_NS

#define ParamCancellable "cancellable"
#define ParamMountOperation "operation"
#define ParamForce "force"

Q_NAMESPACE

enum class DeviceType : uint16_t {
    kAllDevice = 0,
    kBlockDevice = 1,   // normal block devices, like removable disks
    kProtocolDevice = 2,   // protocol devices, like smb/mtp/ftp/ptp etc...
    kNetDevice = 3,
};
Q_ENUM_NS(DeviceType)

enum class NetworkMountPasswdSaveMode : uint8_t {
    kNeverSavePasswd = 0,
    kSaveBeforeLogout,
    kSavePermanently,
};
Q_ENUM_NS(NetworkMountPasswdSaveMode)

enum class MonitorStatus : uint16_t {
    kMonitoring = 0,
    kIdle = 1,

    kNotDefined = 0xff,
};
Q_ENUM_NS(MonitorStatus)

enum class Property : uint16_t {
    kNotInit = 0,
    // blocks property
    kBlockProperty,   // this is invalid but just a placeholder
    kBlockConfiguration,   // variant
    kBlockUserspaceMountOptions,   // strlist
    kBlockCryptoBackingDevice,   // str
    kBlockDevice,
    kBlockDrive,
    kBlockIDLabel,
    kBlockIDType,
    kBlockIDUsage,   // uint64
    kBlockIDUUID,   // str
    kBlockIDVersion,
    kBlockDeviceNumber,   // uint64
    kBlockPreferredDevice,   // str
    kBlockID,
    kBlockSize,   // uint64
    kBlockReadOnly,   // bool
    kBlockSymlinks,   // strlist
    kBlockHintPartitionable,   // bool
    kBlockHintSystem,
    kBlockHintIgnore,
    kBlockHintAuto,
    kBlockHintName,   // str
    kBlockHintIconName,
    kBlockHintSymbolicIconName,
    kBlockMdRaid,
    kBlockMdRaidMember,
    kBlockPropertyEND,

    kDriveProperty = 30,
    kDriveConnectionBus,   // str
    kDriveRemovable,   // bool
    kDriveEjectable,
    kDriveSeat,   // str
    kDriveMedia,
    kDriveMediaCompatibility,   // strlist
    kDriveMediaRemovable,   // bool
    kDriveMediaAvailable,
    kDriveMediaChangeDetected,
    kDriveTimeDetected,   // uint64
    kDriveTimeMediaDetected,
    kDriveSize,
    kDriveOptical,   // bool
    kDriveOpticalBlank,
    kDriveOpticalNumTracks,   // uint
    kDriveOpticalNumAudioTracks,
    kDriveOpticalNumDataTracks,
    kDriveOpticalNumSessions,
    kDriveModel,   // str
    kDriveRevision,
    kDriveRotationRate,   // int
    kDriveSerial,   // str
    kDriveVender,
    kDriveWWN,
    kDriveSortKey,
    kDriveConfiguration,   // variant
    kDriveID,   // str
    kDriveCanPowerOff,   // bool
    kDriveSiblingID,   // str
    kDrivePropertyEND,

    kFileSystemProperty,
    kFileSystemMountPoint,   // strlist
    kFileSystemPropertyEND,

    kPartitionProperty,
    kPartitionNumber,   // uint
    kPartitionType,   // str
    kPartitionOffset,   // uint64
    kPartitionSize,
    kPartitionFlags,
    kPartitionName,   // str
    kPartitionUUID,
    kPartitionTable,
    kPartitionIsContainer,   // bool
    kPartitionIsContained,
    kPartitionPropertyEND,

    kEncryptedProperty,
    kEncryptedChildConfiguration,   // variant
    kEncryptedCleartextDevice,   // str
    kEncryptedHintEncryptionType,
    kEncryptedMetadataSize,   // uint64
    kEncryptedPropertyEnd,

    // protocol property
    kProtoProperty = 800,   // this is invalid but just a placeholder
};
Q_ENUM_NS(Property);

#define UDISKS_ERR_DOMAIN "udisks-error-quark"
#define UDISKS_ERR_START 0
#define GIO_ERR_DOMAIN "g-io-error-quark"
#define GIO_ERR_START 200
#define GDBUS_ERR_DOMAIN "g-dbus-error-quark"
#define GDBUS_ERR_START 400
#define USER_ERR_START 800
enum class DeviceError : uint16_t {
    kNoError = 10000,

    // ￬ these errors are transfered from udisks
    kUDisksErrorFailed = UDISKS_ERR_START,
    kUDisksErrorCancelled,
    kUDisksErrorAlreadyCancelled,
    kUDisksErrorNotAuthorized,
    kUDisksErrorNotAuthorizedCanObtain,
    kUDisksErrorNotAuthorizedDismissed,
    kUDisksErrorAlreadyMounted,
    kUDisksErrorNotMounted,
    kUDisksErrorOptionNotPermitted,
    kUDisksErrorMountedByOtherUser,
    kUDisksErrorAlreadyUnmounting,
    kUDisksErrorNotSupproted,
    kUDisksErrorTimedOut,
    kUDisksErrorWouldWakeup,
    kUDisksErrorDeviceBusy,
    kUDisksErrorScsiDaemonTransportFailed,
    kUDisksErrorScsiHostNotFound,
    kUDisksErrorScsiIDMB,
    kUDisksErrorScsiLoginFailed,
    kUDisksErrorScsiLoginAuthFailed,
    kUDisksErrorScsiLoginFatal,
    kUDisksErrorScsiLogoutFailed,
    kUDisksErrorScsiNoFirmware,
    kUDisksErrorScsiNoObjectsFound,
    kUDisksErrorScsiNotConnected,
    kUDisksErrorScsiTransportFailed,
    kUDisksErrorScsiUnknownDiscoveryType,   // ￪ these errors are transfered from udisks

    kUDisksBusySMARTSelfTesting,
    kUDisksBusyDriveEjecting,
    kUDisksBusyEncryptedUnlocking,
    kUDisksBusyEncryptedLocking,
    kUDisksBusyEncryptedModifying,
    kUDisksBusyEncryptedResizing,
    kUDisksBusySwapSpaceStarting,
    kUDisksBusySwapSpaceStoping,
    kUDisksBusySwpaSpaceModifying,
    kUDisksBusyFileSystemUnmounting,
    kUDisksBusyFileSystemMounting,
    kUDisksBusyFileSystemModifying,
    kUDisksBusyFileSystemResizing,
    kUDisksBusyFormatErasing,
    kUDisksBusyFormatMkfsing,
    kUDisksBusyLoopSetuping,
    kUDisksBusyPartitionModifying,
    kUDisksBusyPartitionDeleting,
    kUDisksBusyPartitionCreating,
    kUDisksBusyCleanuping,
    kUDisksBusyATASecureErasing,
    kUDisksBusyATAEnhancedSecureErasing,
    kUDisksBusyMdRaidStarting,
    kUDisksBusyMdRaidStoping,
    kUDisksBusyMdRaidFaultingDevice,
    kUDisksBusyMdRaidRemovingDevice,
    kUDisksBusyMdRaidCreating,

    // these errors are from GIOErrorEnum
    kGIOError = GIO_ERR_START,
    kGIOErrorFailed = kGIOError,
    kGIOErrorNotFound,
    kGIOErrorExists,
    kGIOErrorIsDirectory,
    kGIOErrorNotDirertory,
    kGIOErrorNotEmpty,
    kGIOErrorNotRegularFile,
    kGIOErrorNotSymbolicLink,
    kGIOErrorNotMountableFile,
    kGIOErrorFilenameTooLong,
    kGIOErrorInvalidFilename,
    kGIOErrorTooManyLinks,
    kGIOErrorNoSpace,
    kGIOErrorInvalidArgument,
    kGIOErrorPermissionDenied,
    kGIOErrorNotSupported,
    kGIOErrorNotMounted,
    kGIOErrorAlreadyMounted,
    kGIOErrorClosed,
    kGIOErrorCancelled,
    kGIOErrorPending,
    kGIOErrorReadONly,
    kGIOErrorCantCreateBackup,
    kGIOErrorWrongETAG,
    kGIOErrorTimedOut,
    kGIOErrorWouldRecurse,
    kGIOErrorBusy,
    kGIOErrorWouldBlock,
    kGIOErrorHostNotFound,
    kGIOErrorWouldMerge,
    kGIOErrorFailedHandled,
    kGIOErrorTooManyOpenFiles,
    kGIOErrorNotInitilized,
    kGIOErrorAddressInUse,
    kGIOErrorPartialInput,
    kGIOErrorInvalidData,
    kGIOErrorDBusError,
    kGIOErrorHostUnreachable,
    kGIOErrorNetworkUnreachable,
    kGIOErrorConnectionRefused,
    kGIOErrorProxyFailed,
    kGIOErrorProxyAuthFailed,
    kGIOErrorProxyNeedAuth,
    kGIOErrorProxyNotAllowed,
    kGIOErrorBrokenPipe,
    kGIOErrorConnectionClosed = kGIOErrorBrokenPipe,
    kGIOErrorNotConnected,
    kGIOErrorMessageTooLarge,

    // these are from GDBusError
    kGDBusError = GDBUS_ERR_START,
    kGDBusErrorFailed = kGDBusError,
    kGDBusErrorNoMemory,
    kGDBusErrorServiceUnknown,
    kGDBusErrorNameHasNoOwner,
    kGDBusErrorNoReply,
    kGDBusErrorIOError,
    kGDBusErrorBadAddress,
    kGDBusErrorNotSupported,
    kGDBusErrorLimitsExceeded,
    kGDBusErrorAccessDenied,
    kGDBusErrorAuthFailed,
    kGDBusErrorNoServer,
    kGDBusErrorTimeout,
    kGDBusErrorNoNetwork,
    kGDBusErrorAddressInUse,
    kGDBusErrorDisconnected,
    kGDBusErrorInvalidArgs,
    kGDBusErrorFileNotFound,
    kGDBusErrorFileExists,
    kGDBusErrorUnknownMethod,
    kGDBusErrorTimedOut,
    kGDBusErrorMatchRuleNotFound,
    kGDBusErrorMatchRuleInvalid,
    kGDBusErrorSpawnExecFailed,
    kGDBusErrorSpawnForkFailed,
    kGDBusErrorSpawnChildExited,
    kGDBusErrorSpawnChildSignaled,
    kGDBusErrorSpawnFailed,
    kGDBusErrorSpawnSetupFailed,
    kGDBusErrorSpawnConfigInvalid,
    kGDBusErrorSpawnServiceInvalid,
    kGDBusErrorSpawnServiceNotFound,
    kGDBusErrorSpawnPermissionsInvalid,
    kGDBusErrorSpawnFileInvalid,
    kGDBusErrorSpawnNoMemory,
    kGDBusErrorUnixProcessIdUnkown,
    kGDBusErrorInvalidSignature,
    kGDBusErrorInvalidFileContent,
    kGDBusErrorSeLinuxSecurityContextUnknown,
    kGDBusErrorADTAuditDataUnknown,
    kGDBusErrorObjectPathInUse,
    kGDBusErrorUnknownObject,
    kGDBusErrorUnknownInterface,
    kGDBusErrorUnknownProperty,
    kGDBusErrorPropertyReadOnly,

    kUserError = USER_ERR_START,
    kUserErrorNotMountable,   // ￬ these are by ourselves
    kUserErrorNotEjectable,
    kUserErrorNoDriver,
    kUserErrorNotEncryptable,
    kUserErrorNoPartition,
    kUserErrorNoBlock,
    kUserErrorNetworkWrongPasswd,
    kUserErrorNetworkAnonymousNotAllowed,
    kUserErrorUserCancelled,
    kUserErrorTimedOut,
    kUserErrorAlreadyMounted,
    kUserErrorNotMounted,
    kUserErrorNotPoweroffable,
    kUserErrorFailed,

    kUnhandledError = 1000,
};
Q_ENUM_NS(DeviceError)

enum class MonitorError : uint16_t {
    kNoError = 0,
    kMonitorAlreadyRegistered,
    kMonitorNotRegister,
};
Q_ENUM_NS(MonitorError)

enum class PartitionType : uint16_t {
    kPartitionTypeNotFound = 65535,

    kMbrEmpty = 0x00,
    kMbrFAT12Type = 0x01,
    kMbrXENIX_root = 0x02,
    kMbrXENIX_usr = 0x03,
    kMbrFAT16_Less_32M = 0x04,
    kMbrExtended = 0x05,
    kMbrFAT16Type = 0x06,
    kMbrHPFS_NTFS = 0x07,
    kMbrAIX = 0x08,
    kMbrAIX_bootable = 0x09,
    kMbrOS2_Boot_Manager = 0x0a,
    kMbrWin95_FAT32 = 0x0b,
    kMbrWin95_FAT32_LBA = 0x0c,
    kMbrWin95_FAT16_LBA = 0x0e,
    kMbrWin95_Extended_LBA = 0x0f,
    kMbrOPUS = 0x10,
    kMbrHidden_FAT12 = 0x11,
    kMbrCompaq_diagnostics = 0x12,
    kMbrHidden_FAT16_Less_32M = 0x14,
    kMbrHidden_FAT16 = 0x16,
    kMbrHidden_HPFS_or_NTFS = 0x17,
    kMbrAST_SmartSleep = 0x18,
    kMbrHidden_Win95_FAT32 = 0x1b,
    kMbrHidden_Win95_FAT32_LBA = 0x1c,
    kMbrHidden_Win95_FAT16_LBA = 0x1e,
    kMbrNEC_DOS = 0x24,
    kMbrPlan9 = 0x39,
    kMbrPartitionMagic_recovery = 0x3c,
    kMbrVenix_80286 = 0x40,
    kMbrPPC_PReP_Boot = 0x41,
    kMbrSFS = 0x42,
    kMbrQNX4_dot_x = 0x4d,
    kMbrQNX4_dot_x_2nd_part = 0x4e,
    kMbrQNX4_dot_x_3rd_part = 0x4f,
    kMbrOnTrack_DM = 0x50,
    kMbrOnTrack_DM6_Aux1 = 0x51,
    kMbrCP_M = 0x52,
    kMbrOnTrack_DM6_Aux3 = 0x53,
    kMbrOnTrackDM6 = 0x54,
    kMbrEZ_Drive = 0x55,
    kMbrGolden_Bow = 0x56,
    kMbrPriam_Edisk = 0x5c,
    kMbrSpeedStor = 0x61,
    kMbrGNU_HURD_or_SysV = 0x63,
    kMbrNovell_Netware_286 = 0x64,
    kMbrNovell_Netware_386 = 0x65,
    kMbrDiskSecure_Multi_Boot = 0x70,
    kMbrPC_IX = 0x75,
    kMbrOld_Minix = 0x80,
    kMbrMinix_old_Linux = 0x81,
    kMbrLinux_swap = 0x82,
    kMbrLinux = 0x83,
    kMbrOS2_hidden_C_drive = 0x84,
    kMbrLinux_extended = 0x85,
    kMbrNTFS_volume_set_1 = 0x86,
    kMbrNTFS_volume_set_2 = 0x87,
    kMbrLinux_LVM = 0x8e,
    kMbrAmoeba = 0x93,
    kMbrAmoeba_BBT = 0x94,
    kMbrBSD_OS = 0x9f,
    kMbrIBM_Thinkpad_hibernation = 0xa0,
    kMbrFreeBSD = 0xa5,
    kMbrOpenBSD = 0xa6,
    kMbrNeXTSTEP = 0xa7,
    kMbrNetBSD = 0xa8,
    kMbrBSDI_fs = 0xa9,
    kMbrBSDI_swap = 0xb7,
    kMbrBoot_Wizard_hidden = 0xb8,
    kMbrDRDOS_sec_FAT12 = 0xbb,
    kMbrDRDOS_sec_FAT16_Less_32M = 0xc1,
    kMbrDRDOS_sec_FAT16 = 0xc4,
    kMbrDRDOS_sec_extend = 0xc5,
    kMbrSyrinx = 0xc6,
    kMbrNon_FS_data = 0xc7,
    kMbrMultiuser_DOS_extend = 0xd5,
    kMbrCP_M_CTOS_dot_dot_dot = 0xda,
    kMbrDell_Utility = 0xdb,
    kMbrBootIt = 0xde,
    kMbrDOS_access = 0xdf,
    kMbrDOS_R_O = 0xe1,
    kMbrSpeedStor_1 = 0xe3,
    kMbrBeOS_fs = 0xe4,
    kMbrEFI_GPT = 0xeb,
    kMbrEFI_FAT12_16_32 = 0xee,
    kMbrLinux_PA_RISC_boot = 0xef,
    kMbrSpeedStor_2 = 0xf0,
    kMbrSeppdStor_3 = 0xf4,
    kMbrDOS_secondary = 0xf2,
    kMbrLinux_raid_autodetect = 0xfd,
    kMbrLANstep = 0xfe,
    kMbrBBT = 0xff,

    // Gpt[Name][FileSystem]
    // https://en.wikipedia.org/wiki/GUID_Partition_Table
    kGptUnusedEntryNA,
    kGptMBRPartitionSchemeNA,
    kGptEFISystemPartitionNA,
    kGptBIOSBootPartitionNA,
    kGptIntelFastFlashPartitionNA,
    kGptSonyBootPartitionNA,
    kGptLenovoBootPartitionNA,

    kGptMicrosoftReservedPartitionWin,
    kGptBasicDataPartitionWin,
    kGptLogicalDiskManagerMetaDataPartitionWin,
    kGptLogicalDiskManagerDataPartitionWin,
    kGptWindowsRecoveryEnvironmentWin,
    kGptIBMGeneralParallelFileSystemPartitionWin,
    kGptStorageSpacesPartitionWin,
    kGptStorageReplicaPartitionWin,

    kGptDataPartitionHPUX,
    kGptServicePartitionHPUX,

    kGptLinuxFilesystemDataLinux,
    kGptRAIDPartitionLinux,
    kGptRootPartitionX86Linux,
    kGptRootPartitionX8664Linux,
    kGptRootPartitionArm32Linux,
    kGptRootPartitionArm64Linux,
    kGptBootPartitionLinux,
    kGptSwapPartitionLinux,
    kGptLogicalVolumeManagerPartitionLinux,
    kGptHomePartitionLinux,
    kGptServerDataPartitionLinux,
    kGptPlainDMCryptPartitionLinux,
    kGptLUKSPartitionLinux,
    kGptReservedLinux,

    kGptBootPartitionFreeBSD,
    kGptBSDDisklabelPartitionFreeBSD,
    kGptSwapPartitionFreeBSD,
    kGptUnixFileSystemPartitionFreeBSD,
    kGptVinumVolumeManagerPartitionFreeBSD,
    kGptZFSPartitionFreeBSD,
    kGptNandfsPartitionFreeBSD,

    kGptHierarchialFileSystemPlusPartitionMacOS,
    kGptAppleAPFSContainerMacOS,
    kGptAppleUFSContainerMacOS,
    kGptZFSMacOS,
    kGptAppleRAIDPartitionMacOS,
    kGptAppleRAIDPartitionOfflineMacOS,
    kGptAppleBootPartitionMacOS,
    kGptAppleLabelMacOS,
    kGptAppleTVRecoveryPartitionMacOS,
    kGptAppleCoreStorageContainerMacOS,
    kGptAppleAPFSPrebootPartitionMacOS,
    kGptAppleAPFSRecoveryPartitionMacOS,

    kGptBootPartitionSolaris,
    kGptRootPartitionSolaris,
    kGptSwapPartitionSolaris,
    kGptBackupPartitionSolaris,
    kGptUsrPartitionSolaris,
    kGptVarPartitionSolaris,
    kGptHomePartitionSolaris,
    kGptAlternateSectorSolaris,
    kGptReservedPartitionSolaris,

    kGptSwapPartitionNetBSD,
    kGptFFSPartitionNetBSD,
    kGptLFSPartitionNetBSD,
    kGptRAIDPartitionNetBSD,
    kGptConcatenatedPartitionNetBSD,
    kGptEncryptedPartitionNetBSD,

    kGptKernelChromeOS,
    kGptRootfsChromeOS,
    kGptFirmwareChromeOS,
    kGptFutureUseChromeOS,
    kGptMiniOSChromeOS,
    kGptHibernateChromeOS,

    kGptUsrPartitionCoreOS,
    kGptResizableRootfsCoreOS,
    kGptOEMCustomizationsCoreOS,
    kGptRootFilesystemOnRAIDCoreOS,

    kGptHaikuBFSHaiku,

    kGptBootPartitionMidnightBSD,
    kGptDataPartitionMidnightBSD,
    kGptSwapPartitionMidnightBSD,
    kGptUnixFileSystemPartitionMidnightBSD,
    kGptVinumVolumemanagerPartitionMidnightBSD,
    kGptZFSPartitionMidnightBSD,

    kGptJournalCeph,
    kGptDmCryptJournalCeph,
    kGptOSDCeph,
    kGptDmCryptOSDCeph,
    kGptDiskinCreationCeph,
    kGptDmCryptDiskinCreationCeph,
    kGptBlockCeph,
    kGptBlockDBCeph,
    kGptBlockWriteAheadlogCeph,
    kGptLockboxForDmCryptKeysCeph,
    kGptMultipathOSDCeph,
    kGptMultipathJournalCeph,
    kGptMultipathBlockCeph,
    kGptMultipathBlockDBCeph,
    kGptMultipathblockwriteAheadogCeph,
    kGptDmCryptBlockCeph,
    kGptDmCryptBlockDBCeph,
    kGptDmCryptBlockWriteAheadlogCeph,
    kGptDmCryptLUKSjournalCeph,
    kGptDmCryptLUKSBlockCeph,
    kGptDmCryptLUKSBlockDBCeph,
    kGptDmCryptLUKSBlockwriteAheadlogCeph,
    kGptDmCryptLUKSOSDCeph,

    kGptDataPartitionOpenBSD,

    kGptPowerSafeFilesystemQNX,

    kGptPlan9PartitionPlan9,

    kGptVmkCoreVMwareESX,
    kGptVMFSFilesystemPartitionVMwareESX,
    kGptVMWareReservedVMwareESX,

    kGptBootloaderAndroidIA,
    kGptBootloader2AndroidIA,
    kGptBootAndroidIA,
    kGptRecoveryAndroidIA,
    kGptMiscAndroidIA,
    kGptMetadataAndroidIA,
    kGptSystemAndroidIA,
    kGptCacheAndroidIA,
    kGptDataAndroidIA,
    kGptPersistentAndroidIA,
    kGptVendorAndroidIA,
    kGptConfigAndroidIA,
    kGptFactoryAndroidIA,
    kGptFactoryAltAndroidIA,
    kGptFastbootOrTertiaryAndroidIA,
    kGptOEMAndroidIA,

    kGptAndroidMetaAndroid6Arm,
    kGptAndroidEXTAndroid6Arm,

    kGptBootONIE,
    kGptConfigONIE,

    kGptPRePBootPowerPC,

    kGptSharedBootloaderConfigurationFreedesktop,

    kGptBasicDataPartitionAtariTOS,

    kGptEncryptedDataPartitionVeraCrypt,

    kGptArcaOSType1OS2,

    kGptSPDKBlockDeviceSPDK,

    kGptBareBoxStateBareboxBootloader,

    kGptUBootEnvironmentUBootBootloader,

    kGptStatusSoftRAID,
    kGptScratchSoftRAID,
    kGptVolumeSoftRAID,
    kGptCacheSoftRAID,

    kGptBootloaderFuchsiaStandard,
    kGptDurablemutableencryptedsystemdataFuchsiaStandard,
    kGptDurablemutablebootloaderdataFuchsiaStandard,
    kGptFactoryProvisionedreadOnlysystemdataFuchsiaStandard,
    kGptFactoryProvisionedreadOnlybootloaderdataFuchsiaStandard,
    kGptFuchsiaVolumeManagerFuchsiaStandard,
    kGptVerifiedbootmetadataFuchsiaStandard,
    kGptZirconbootimageFuchsiaStandard,

    kGptFuchsiaEspFuchsiaLegacy,
    kGptFuchsiaSystemFuchsiaLegacy,
    kGptFuchsiaDataFuchsiaLegacy,
    kGptFuchsiaInstallFuchsiaLegacy,
    kGptFuchsiaBlobFuchsiaLegacy,
    kGptFuchsiaFvmFuchsiaLegacy,
    kGptZirconbootimageSlotAFuchsiaLegacy,
    kGptZirconbootimageSlotBFuchsiaLegacy,
    kGptZirconbootimageSlotRFuchsiaLegacy,
    kGptSysConfigFuchsiaLegacy,
    kGptFactoryConfigFuchsiaLegacy,
    kGptBootloaderFuchsiaLegacy,
    kGptGuidTestFuchsiaLegacy,
    kGptVerifiedbootmetadataSlotAFuchsiaLegacy,
    kGptVerifiedbootmetadataSlotBFuchsiaLegacy,
    kGptVerifiedbootmetadataSlotRFuchsiaLegacy,
    kGptMiscFuchsiaLegacy,
    kGptEmmcBoot1FuchsiaLegacy,
    kGptEmmcBoot2FuchsiaLegacy,
};
Q_ENUM_NS(PartitionType)

using DeviceOperateCallback = std::function<void(bool, DeviceError)>;
using DeviceOperateCallbackWithMessage = std::function<void(bool, DeviceError, QString)>;

DFM_MOUNT_END_NS
#endif   // DMOUNT_GLOBAL_H
