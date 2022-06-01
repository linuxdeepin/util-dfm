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

#include "base/dmountutils.h"

#include <QDebug>

extern "C" {
#include <glib.h>
}
#include <unistd.h>
#include <pwd.h>

DFM_MOUNT_USE_NS

// use G_VARIANT_TYPE_* gets alot compile warning
#define GVAR_TYPE_BOOL reinterpret_cast<const GVariantType *>("b")
#define GVAR_TYPE_BYTE reinterpret_cast<const GVariantType *>("y")
#define GVAR_TYPE_INT16 reinterpret_cast<const GVariantType *>("n")
#define GVAR_TYPE_UINT16 reinterpret_cast<const GVariantType *>("q")
#define GVAR_TYPE_INT32 reinterpret_cast<const GVariantType *>("i")
#define GVAR_TYPE_UINT32 reinterpret_cast<const GVariantType *>("u")
#define GVAR_TYPE_INT64 reinterpret_cast<const GVariantType *>("x")
#define GVAR_TYPE_UINT64 reinterpret_cast<const GVariantType *>("t")
#define GVAR_TYPE_DOUBLE reinterpret_cast<const GVariantType *>("d")
#define GVAR_TYPE_STRING reinterpret_cast<const GVariantType *>("s")
#define GVAR_TYPE_STRING_ARR reinterpret_cast<const GVariantType *>("as")
#define GVAR_TYPE_OBJECTPATH reinterpret_cast<const GVariantType *>("o")
#define GVAR_TYPE_OBJECTPATH_ARR reinterpret_cast<const GVariantType *>("ao")
#define GVAR_TYPE_VARIANT reinterpret_cast<const GVariantType *>("v")
#define GVAR_TYPE_BYTESTRING reinterpret_cast<const GVariantType *>("ay")
#define GVAR_TYPE_BYTESTRING_ARR reinterpret_cast<const GVariantType *>("aay")
#define GVAR_TYPE_VARDICT reinterpret_cast<const GVariantType *>("a{sv}")
#define GVAR_TYPE_ARR reinterpret_cast<const GVariantType *>("a*")

QVariant Utils::castFromGVariant(GVariant *val)
{
    auto isType = [val](const GVariantType *type) {
        return g_variant_is_of_type(val, type);
    };
    if (!val)
        return QVariant();
    if (isType(GVAR_TYPE_BOOL))
        return QVariant(static_cast<bool>(g_variant_get_boolean(val)));
    if (isType(GVAR_TYPE_BYTE))
        return QVariant(static_cast<char>(g_variant_get_byte(val)));
    if (isType(GVAR_TYPE_INT16))
        return QVariant(static_cast<int>(g_variant_get_int16(val)));
    if (isType(GVAR_TYPE_INT32))
        return QVariant(static_cast<int>(g_variant_get_int32(val)));
    if (isType(GVAR_TYPE_INT64))
        return QVariant(static_cast<qint64>(g_variant_get_int64(val)));
    if (isType(GVAR_TYPE_UINT16))
        return QVariant(static_cast<uint>(g_variant_get_uint16(val)));
    if (isType(GVAR_TYPE_UINT32))
        return QVariant(static_cast<uint>(g_variant_get_uint32(val)));
    if (isType(GVAR_TYPE_UINT64))
        return QVariant(static_cast<quint64>(g_variant_get_uint64(val)));
    if (isType(GVAR_TYPE_DOUBLE))
        return QVariant(static_cast<double>(g_variant_get_double(val)));
    if (isType(GVAR_TYPE_STRING) || isType(GVAR_TYPE_OBJECTPATH))
        return QString(g_variant_get_string(val, nullptr));
    if (isType(GVAR_TYPE_VARIANT))
        return castFromGVariant(val);
    if (isType(GVAR_TYPE_STRING_ARR)) {
        QStringList ret;
        const char **strv = g_variant_get_strv(val, nullptr);
        int next = 0;
        while (strv && strv[next]) {
            ret << QString(strv[next]);
            next++;
        }
        g_free(strv);
        return ret;
    }
    if (isType(GVAR_TYPE_OBJECTPATH_ARR)) {
        QStringList ret;
        const char **strv = g_variant_get_objv(val, nullptr);
        int next = 0;
        while (strv && strv[next]) {
            ret << QString(strv[next]);
            next++;
        }
        g_free(strv);
        return ret;
    }
    if (isType(GVAR_TYPE_BYTESTRING)) {
        const char *byteString = g_variant_get_bytestring(val);
        return QString(byteString);
    }
    if (isType(GVAR_TYPE_BYTESTRING_ARR)) {
        QStringList ret;
        const char **bstrv = g_variant_get_bytestring_array(val, nullptr);
        int next = 0;
        while (bstrv && bstrv[next]) {
            ret << QString(bstrv[next]);
            next++;
        }
        g_free(bstrv);
        return ret;
    }
    if (isType(GVAR_TYPE_VARDICT)) {
        QVariantMap ret;
        GVariantIter *iter = nullptr;
        g_variant_get(val, "a{sv}", &iter);
        char *key = nullptr;
        GVariant *item = nullptr;
        while (iter && g_variant_iter_next(iter, "{&sv}", &key, &item))
            ret.insert(QString(key), castFromGVariant(item));
        g_variant_iter_free(iter);
        return ret;
    }
    if (isType(GVAR_TYPE_ARR)) {
        QList<QVariant> lst;
        GVariantIter *iter = nullptr;
        g_variant_get(val, "av", &iter);
        GVariant *item = nullptr;
        while (iter && g_variant_iter_loop(iter, "v", &item))
            lst << castFromGVariant(item);
        g_variant_iter_free(iter);
        return lst;
    }
    qDebug() << g_variant_classify(val) << "cannot be parsed";
    return QVariant();
}

GVariant *Utils::castFromQVariant(const QVariant &val)
{
    switch (val.type()) {
    case QVariant::Bool:
        return g_variant_new("b", val.toBool());
    case QVariant::Int:
        return g_variant_new("i", val.toInt());
    case QVariant::UInt:
        return g_variant_new("u", val.toUInt());
    case QVariant::LongLong:
        return g_variant_new("x", val.toLongLong());
    case QVariant::ULongLong:
        return g_variant_new("t", val.toULongLong());
    case QVariant::Double:
        return g_variant_new("d", val.toDouble());
    case QVariant::Char:
        return g_variant_new("y", val.toChar().toLatin1());
    case QVariant::String: {
        std::string str = val.toString().toStdString();
        const char *cstr = str.c_str();
        return g_variant_new("s", cstr);
    }
    case QVariant::StringList:
        return castFromQStringList(val.toStringList());
    case QVariant::ByteArray:
        return g_variant_new_bytestring(val.toByteArray().data());
    case QVariant::Map:
        return castFromQVariantMap(val.toMap());
    case QVariant::List:
        return castFromList(val.toList());
    default:
        return nullptr;
    }
}

GVariant *Utils::castFromQVariantMap(const QVariantMap &val)
{
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    if (!builder) {
        qWarning() << "cannot allocate a gvariantbuilder";
        return nullptr;
    }

    for (auto iter = val.cbegin(); iter != val.cend(); iter += 1) {
        GVariant *item = castFromQVariant(iter.value());
        std::string key = iter.key().toStdString();
        const char *ckey = key.c_str();
        if (item)
            g_variant_builder_add(builder, "{sv}", ckey, item);
    }

    GVariant *ret = g_variant_builder_end(builder);
    g_variant_builder_unref(builder);
    return ret;
}

GVariant *Utils::castFromQStringList(const QStringList &val)
{
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
    if (!builder) {
        qWarning() << "cannot allocate a gvariantbuilder";
        return nullptr;
    }

    for (const auto &s : val) {
        std::string str = s.toStdString();
        const char *cstr = str.c_str();
        g_variant_builder_add(builder, "s", cstr);
    }

    GVariant *ret = g_variant_builder_end(builder);
    g_variant_builder_unref(builder);
    return ret;
}

GVariant *Utils::castFromList(const QList<QVariant> &val)
{
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("av"));
    if (!builder) {
        qWarning() << "cannot allocate a gvariantbuilder";
        return nullptr;
    }

    for (auto valItem : val) {
        GVariant *item = castFromQVariant(valItem);
        if (item)
            g_variant_builder_add(builder, "v", item);
    }

    GVariant *ret = g_variant_builder_end(builder);
    g_variant_builder_unref(builder);
    return ret;
}

Property Utils::getPropertyByName(const QString &name, const QString &iface)
{
#ifdef p
#    undef p
#endif
#define p(k, v) std::pair<QString, Property>(k, v)
    const static QMap<QString, Property> datas = {
        p("UserspaceMountOptions", Property::kBlockUserspaceMountOptions),
        p("Configuration", Property::kBlockConfiguration),
        p("CryptoBackingDevice", Property::kBlockCryptoBackingDevice),
        p("Device", Property::kBlockDevice),
        p("Drive", Property::kBlockDrive),
        p("IdLabel", Property::kBlockIDLabel),
        p("IdType", Property::kBlockIDType),
        p("IdUsage", Property::kBlockIDUsage),
        p("IdUUID", Property::kBlockIDUUID),
        p("IdVersion", Property::kBlockIDVersion),
        p("DeviceNumber", Property::kBlockDeviceNumber),
        p("PreferredDevice", Property::kBlockPreferredDevice),
        p("Id", Property::kBlockID),
        p("Size", Property::kBlockSize),
        p("ReadOnly", Property::kBlockReadOnly),
        p("Symlinks", Property::kBlockSymlinks),
        p("HintPartitionable", Property::kBlockHintPartitionable),
        p("HintSystem", Property::kBlockHintSystem),
        p("HintIgnore", Property::kBlockHintIgnore),
        p("HintAuto", Property::kBlockHintAuto),
        p("HintName", Property::kBlockHintName),
        p("HintIconName", Property::kBlockHintIconName),
        p("HintSymbolicIconName", Property::kBlockHintSymbolicIconName),
        p("MdRaid", Property::kBlockMdRaid),
        p("MdRaidMember", Property::kBlockMdRaidMember),
        p("ConnectionBus", Property::kDriveConnectionBus),
        p("Removable", Property::kDriveRemovable),
        p("Ejectable", Property::kDriveEjectable),
        p("Seat", Property::kDriveSeat),
        p("Media", Property::kDriveMedia),
        p("MediaCompatibility", Property::kDriveMediaCompatibility),
        p("MediaRemovable", Property::kDriveMediaRemovable),
        p("MediaAvailable", Property::kDriveMediaAvailable),
        p("MediaChangeDetected", Property::kDriveMediaChangeDetected),
        p("TimeDetected", Property::kDriveTimeDetected),
        p("TimeMediaDetected", Property::kDriveTimeMediaDetected),
        p("Size", Property::kDriveSize),
        p("Optical", Property::kDriveOptical),
        p("OpticalBlank", Property::kDriveOpticalBlank),
        p("OpticalNumTracks", Property::kDriveOpticalNumTracks),
        p("OpticalNumAudioTracks", Property::kDriveOpticalNumAudioTracks),
        p("OpticalNumDataTracks", Property::kDriveOpticalNumDataTracks),
        p("OpticalNumSessions", Property::kDriveOpticalNumSessions),
        p("Model", Property::kDriveModel),
        p("Revision", Property::kDriveRevision),
        p("RotationRate", Property::kDriveRotationRate),
        p("Serial", Property::kDriveSerial),
        p("Vender", Property::kDriveVender),
        p("WWN", Property::kDriveWWN),
        p("SortKey", Property::kDriveSortKey),
        p("Configuration", Property::kDriveConfiguration),
        p("ID", Property::kDriveID),
        p("CanPowerOff", Property::kDriveCanPowerOff),
        p("SiblingID", Property::kDriveSiblingID),
        p("MountPoints", Property::kFileSystemMountPoint),
        p("Number", Property::kPartitionNumber),
        p("Type", Property::kPartitionType),
        p("Offset", Property::kPartitionOffset),
        p("Size", Property::kPartitionSize),
        p("Flags", Property::kPartitionFlags),
        p("Name", Property::kPartitionName),
        p("UUID", Property::kPartitionUUID),
        p("Table", Property::kPartitionTable),
        p("IsContainer", Property::kPartitionIsContainer),
        p("IsContained", Property::kPartitionIsContained),
        p("ChildConfiguration", Property::kEncryptedChildConfiguration),
        p("CleartextDevice", Property::kEncryptedCleartextDevice),
        p("HintEncryptionType", Property::kEncryptedHintEncryptionType),
        p("MetadataSize", Property::kEncryptedMetadataSize),
    };

    // there are 3 same-name property in udisks, so distinguish it by InterfaceName
    if (name == "Size") {
        if (iface.endsWith("Block"))
            return Property::kBlockSize;
        else if (iface.endsWith("Drive"))
            return Property::kDriveSize;
        else if (iface.endsWith("Partition"))
            return Property::kPartitionSize;
    }

    return datas.value(name, Property::kNotInit);
}

PartitionType Utils::getPartitionTypeByGuid(const QString &guid)
{
#ifdef p
#    undef p
#endif
#define p(k, v) std::pair<QString, PartitionType>(k, v)

    static const QMap<QString, PartitionType> datas = {
        p("00000000-0000-0000-0000-000000000000", PartitionType::kGptUnusedEntryNA),
        p("024DEE41-33E7-11D3-9D69-0008C781F39F", PartitionType::kGptMBRPartitionSchemeNA),
        p("C12A7328-F81F-11D2-BA4B-00A0C93EC93B", PartitionType::kGptEFISystemPartitionNA),
        p("21686148-6449-6E6F-744E-656564454649", PartitionType::kGptBIOSBootPartitionNA),
        p("D3BFE2DE-3DAF-11DF-BA40-E3A556D89593", PartitionType::kGptIntelFastFlashPartitionNA),
        p("F4019732-066E-4E12-8273-346C5641494F", PartitionType::kGptSonyBootPartitionNA),
        p("BFBFAFE7-A34F-448A-9A5B-6213EB736C22", PartitionType::kGptLenovoBootPartitionNA),
        p("E3C9E316-0B5C-4DB8-817D-F92DF00215AE", PartitionType::kGptMicrosoftReservedPartitionWin),
        p("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", PartitionType::kGptBasicDataPartitionWin),
        p("5808C8AA-7E8F-42E0-85D2-E1E90434CFB3", PartitionType::kGptLogicalDiskManagerMetaDataPartitionWin),
        p("AF9B60A0-1431-4F62-BC68-3311714A69AD", PartitionType::kGptLogicalDiskManagerDataPartitionWin),
        p("DE94BBA4-06D1-4D40-A16A-BFD50179D6AC", PartitionType::kGptWindowsRecoveryEnvironmentWin),
        p("37AFFC90-EF7D-4E96-91C3-2D7AE055B174", PartitionType::kGptIBMGeneralParallelFileSystemPartitionWin),
        p("E75CAF8F-F680-4CEE-AFA3-B001E56EFC2D", PartitionType::kGptStorageSpacesPartitionWin),
        p("558D43C5-A1AC-43C0-AAC8-D1472B2923D1", PartitionType::kGptStorageReplicaPartitionWin),
        p("75894C1E-3AEB-11D3-B7C1-7B03A0000000", PartitionType::kGptDataPartitionHPUX),
        p("E2A1E728-32E3-11D6-A682-7B03A0000000", PartitionType::kGptServicePartitionHPUX),
        p("0FC63DAF-8483-4772-8E79-3D69D8477DE4", PartitionType::kGptLinuxFilesystemDataLinux),
        p("A19D880F-05FC-4D3B-A006-743F0F84911E", PartitionType::kGptRAIDPartitionLinux),
        p("44479540-F297-41B2-9AF7-D131D5F0458A", PartitionType::kGptRootPartitionX86Linux),
        p("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709", PartitionType::kGptRootPartitionX8664Linux),
        p("69DAD710-2CE4-4E3C-B16C-21A1D49ABED3", PartitionType::kGptRootPartitionArm32Linux),
        p("B921B045-1DF0-41C3-AF44-4C6F280D3FAE", PartitionType::kGptRootPartitionArm64Linux),
        p("BC13C2FF-59E6-4262-A352-B275FD6F7172", PartitionType::kGptBootPartitionLinux),
        p("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F", PartitionType::kGptSwapPartitionLinux),
        p("E6D6D379-F507-44C2-A23C-238F2A3DF928", PartitionType::kGptLogicalVolumeManagerPartitionLinux),
        p("933AC7E1-2EB4-4F13-B844-0E14E2AEF915", PartitionType::kGptHomePartitionLinux),
        p("3B8F8425-20E0-4F3B-907F-1A25A76F98E8", PartitionType::kGptServerDataPartitionLinux),
        p("7FFEC5C9-2D00-49B7-8941-3EA10A5586B7", PartitionType::kGptPlainDMCryptPartitionLinux),
        p("CA7D7CCB-63ED-4C53-861C-1742536059CC", PartitionType::kGptLUKSPartitionLinux),
        p("8DA63339-0007-60C0-C436-083AC8230908", PartitionType::kGptReservedLinux),
        p("83BD6B9D-7F41-11DC-BE0B-001560B84F0F", PartitionType::kGptBootPartitionFreeBSD),
        p("516E7CB4-6ECF-11D6-8FF8-00022D09712B", PartitionType::kGptBSDDisklabelPartitionFreeBSD),
        p("516E7CB5-6ECF-11D6-8FF8-00022D09712B", PartitionType::kGptSwapPartitionFreeBSD),
        p("516E7CB6-6ECF-11D6-8FF8-00022D09712B", PartitionType::kGptUnixFileSystemPartitionFreeBSD),
        p("516E7CB8-6ECF-11D6-8FF8-00022D09712B", PartitionType::kGptVinumVolumeManagerPartitionFreeBSD),
        p("516E7CBA-6ECF-11D6-8FF8-00022D09712B", PartitionType::kGptZFSPartitionFreeBSD),
        p("74BA7DD9-A689-11E1-BD04-00E081286ACF", PartitionType::kGptNandfsPartitionFreeBSD),
        p("48465300-0000-11AA-AA11-00306543ECAC", PartitionType::kGptHierarchialFileSystemPlusPartitionMacOS),
        p("7C3457EF-0000-11AA-AA11-00306543ECAC", PartitionType::kGptAppleAPFSContainerMacOS),
        p("55465300-0000-11AA-AA11-00306543ECAC", PartitionType::kGptAppleUFSContainerMacOS),
        p("6A898CC3-1DD2-11B2-99A6-080020736631", PartitionType::kGptZFSMacOS),
        p("52414944-0000-11AA-AA11-00306543ECAC", PartitionType::kGptAppleRAIDPartitionMacOS),
        p("52414944-5F4F-11AA-AA11-00306543ECAC", PartitionType::kGptAppleRAIDPartitionOfflineMacOS),
        p("426F6F74-0000-11AA-AA11-00306543ECAC", PartitionType::kGptAppleBootPartitionMacOS),
        p("4C616265-6C00-11AA-AA11-00306543ECAC", PartitionType::kGptAppleLabelMacOS),
        p("5265636F-7665-11AA-AA11-00306543ECAC", PartitionType::kGptAppleTVRecoveryPartitionMacOS),
        p("53746F72-6167-11AA-AA11-00306543ECAC", PartitionType::kGptAppleCoreStorageContainerMacOS),
        p("69646961-6700-11AA-AA11-00306543ECAC", PartitionType::kGptAppleAPFSPrebootPartitionMacOS),
        p("52637672-7900-11AA-AA11-00306543ECAC", PartitionType::kGptAppleAPFSRecoveryPartitionMacOS),
        p("6A82CB45-1DD2-11B2-99A6-080020736631", PartitionType::kGptBootPartitionSolaris),
        p("6A85CF4D-1DD2-11B2-99A6-080020736631", PartitionType::kGptRootPartitionSolaris),
        p("6A87C46F-1DD2-11B2-99A6-080020736631", PartitionType::kGptSwapPartitionSolaris),
        p("6A8B642B-1DD2-11B2-99A6-080020736631", PartitionType::kGptBackupPartitionSolaris),
        p("6A898CC3-1DD2-11B2-99A6-080020736631", PartitionType::kGptUsrPartitionSolaris),
        p("6A8EF2E9-1DD2-11B2-99A6-080020736631", PartitionType::kGptVarPartitionSolaris),
        p("6A90BA39-1DD2-11B2-99A6-080020736631", PartitionType::kGptHomePartitionSolaris),
        p("6A9283A5-1DD2-11B2-99A6-080020736631", PartitionType::kGptAlternateSectorSolaris),
        p("6A945A3B-1DD2-11B2-99A6-080020736631", PartitionType::kGptReservedPartitionSolaris),
        p("6A9630D1-1DD2-11B2-99A6-080020736631", PartitionType::kGptReservedPartitionSolaris),
        p("6A980767-1DD2-11B2-99A6-080020736631", PartitionType::kGptReservedPartitionSolaris),
        p("6A96237F-1DD2-11B2-99A6-080020736631", PartitionType::kGptReservedPartitionSolaris),
        p("6A8D2AC7-1DD2-11B2-99A6-080020736631", PartitionType::kGptReservedPartitionSolaris),
        p("49F48D32-B10E-11DC-B99B-0019D1879648", PartitionType::kGptSwapPartitionNetBSD),
        p("49F48D5A-B10E-11DC-B99B-0019D1879648", PartitionType::kGptFFSPartitionNetBSD),
        p("49F48D82-B10E-11DC-B99B-0019D1879648", PartitionType::kGptLFSPartitionNetBSD),
        p("49F48DAA-B10E-11DC-B99B-0019D1879648", PartitionType::kGptRAIDPartitionNetBSD),
        p("2DB519C4-B10F-11DC-B99B-0019D1879648", PartitionType::kGptConcatenatedPartitionNetBSD),
        p("2DB519EC-B10F-11DC-B99B-0019D1879648", PartitionType::kGptEncryptedPartitionNetBSD),
        p("FE3A2A5D-4F32-41A7-B725-ACCC3285A309", PartitionType::kGptKernelChromeOS),
        p("3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC", PartitionType::kGptRootfsChromeOS),
        p("CAB6E88E-ABF3-4102-A07A-D4BB9BE3C1D3", PartitionType::kGptFirmwareChromeOS),
        p("2E0A753D-9E48-43B0-8337-B15192CB1B5E", PartitionType::kGptFutureUseChromeOS),
        p("09845860-705F-4BB5-B16C-8A8A099CAF52", PartitionType::kGptMiniOSChromeOS),
        p("3F0F8318-F146-4E6B-8222-C28C8F02E0D5", PartitionType::kGptHibernateChromeOS),
        p("5DFBF5F4-2848-4BAC-AA5E-0D9A20B745A6", PartitionType::kGptUsrPartitionCoreOS),
        p("3884DD41-8582-4404-B9A8-E9B84F2DF50E", PartitionType::kGptResizableRootfsCoreOS),
        p("C95DC21A-DF0E-4340-8D7B-26CBFA9A03E0", PartitionType::kGptOEMCustomizationsCoreOS),
        p("BE9067B9-EA49-4F15-B4F6-F36F8C9E1818", PartitionType::kGptRootFilesystemOnRAIDCoreOS),
        p("42465331-3BA3-10F1-802A-4861696B7521", PartitionType::kGptHaikuBFSHaiku),
        p("85D5E45E-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::kGptBootPartitionMidnightBSD),
        p("85D5E45A-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::kGptDataPartitionMidnightBSD),
        p("85D5E45B-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::kGptSwapPartitionMidnightBSD),
        p("0394EF8B-237E-11E1-B4B3-E89A8F7FC3A7", PartitionType::kGptUnixFileSystemPartitionMidnightBSD),
        p("85D5E45C-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::kGptVinumVolumemanagerPartitionMidnightBSD),
        p("85D5E45D-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::kGptZFSPartitionMidnightBSD),
        p("45B0969E-9B03-4F30-B4C6-B4B80CEFF106", PartitionType::kGptJournalCeph),
        p("45B0969E-9B03-4F30-B4C6-5EC00CEFF106", PartitionType::kGptDmCryptJournalCeph),
        p("4FBD7E29-9D25-41B8-AFD0-062C0CEFF05D", PartitionType::kGptOSDCeph),
        p("4FBD7E29-9D25-41B8-AFD0-5EC00CEFF05D", PartitionType::kGptDmCryptOSDCeph),
        p("89C57F98-2FE5-4DC0-89C1-F3AD0CEFF2BE", PartitionType::kGptDiskinCreationCeph),
        p("89C57F98-2FE5-4DC0-89C1-5EC00CEFF2BE", PartitionType::kGptDmCryptDiskinCreationCeph),
        p("CAFECAFE-9B03-4F30-B4C6-B4B80CEFF106", PartitionType::kGptBlockCeph),
        p("30CD0809-C2B2-499C-8879-2D6B78529876", PartitionType::kGptBlockDBCeph),
        p("5CE17FCE-4087-4169-B7FF-056CC58473F9", PartitionType::kGptBlockWriteAheadlogCeph),
        p("FB3AABF9-D25F-47CC-BF5E-721D1816496B", PartitionType::kGptLockboxForDmCryptKeysCeph),
        p("4FBD7E29-8AE0-4982-BF9D-5A8D867AF560", PartitionType::kGptMultipathOSDCeph),
        p("45B0969E-8AE0-4982-BF9D-5A8D867AF560", PartitionType::kGptMultipathJournalCeph),
        p("CAFECAFE-8AE0-4982-BF9D-5A8D867AF560", PartitionType::kGptMultipathBlockCeph),
        p("7F4A666A-16F3-47A2-8445-152EF4D03F6C", PartitionType::kGptMultipathBlockCeph),
        p("EC6D6385-E346-45DC-BE91-DA2A7C8B3261", PartitionType::kGptMultipathBlockDBCeph),
        p("01B41E1B-002A-453C-9F17-88793989FF8F", PartitionType::kGptMultipathblockwriteAheadogCeph),
        p("CAFECAFE-9B03-4F30-B4C6-5EC00CEFF106", PartitionType::kGptDmCryptBlockCeph),
        p("93B0052D-02D9-4D8A-A43B-33A3EE4DFBC3", PartitionType::kGptDmCryptBlockDBCeph),
        p("306E8683-4FE2-4330-B7C0-00A917C16966", PartitionType::kGptDmCryptBlockWriteAheadlogCeph),
        p("45B0969E-9B03-4F30-B4C6-35865CEFF106", PartitionType::kGptDmCryptLUKSjournalCeph),
        p("CAFECAFE-9B03-4F30-B4C6-35865CEFF106", PartitionType::kGptDmCryptLUKSBlockCeph),
        p("166418DA-C469-4022-ADF4-B30AFD37F176", PartitionType::kGptDmCryptLUKSBlockDBCeph),
        p("86A32090-3647-40B9-BBBD-38D8C573AA86", PartitionType::kGptDmCryptLUKSBlockwriteAheadlogCeph),
        p("4FBD7E29-9D25-41B8-AFD0-35865CEFF05D", PartitionType::kGptDmCryptLUKSOSDCeph),
        p("824CC7A0-36A8-11E3-890A-952519AD3F61", PartitionType::kGptDataPartitionOpenBSD),
        p("CEF5A9AD-73BC-4601-89F3-CDEEEEE321A1", PartitionType::kGptPowerSafeFilesystemQNX),
        p("C91818F9-8025-47AF-89D2-F030D7000C2C", PartitionType::kGptPlan9PartitionPlan9),
        p("9D275380-40AD-11DB-BF97-000C2911D1B8", PartitionType::kGptVmkCoreVMwareESX),
        p("AA31E02A-400F-11DB-9590-000C2911D1B8", PartitionType::kGptVMFSFilesystemPartitionVMwareESX),
        p("9198EFFC-31C0-11DB-8F78-000C2911D1B8", PartitionType::kGptVMWareReservedVMwareESX),
        p("2568845D-2332-4675-BC39-8FA5A4748D15", PartitionType::kGptBootloaderAndroidIA),
        p("114EAFFE-1552-4022-B26E-9B053604CF84", PartitionType::kGptBootloader2AndroidIA),
        p("49A4D17F-93A3-45C1-A0DE-F50B2EBE2599", PartitionType::kGptBootAndroidIA),
        p("4177C722-9E92-4AAB-8644-43502BFD5506", PartitionType::kGptRecoveryAndroidIA),
        p("EF32A33B-A409-486C-9141-9FFB711F6266", PartitionType::kGptMiscAndroidIA),
        p("20AC26BE-20B7-11E3-84C5-6CFDB94711E9", PartitionType::kGptMetadataAndroidIA),
        p("38F428E6-D326-425D-9140-6E0EA133647C", PartitionType::kGptSystemAndroidIA),
        p("A893EF21-E428-470A-9E55-0668FD91A2D9", PartitionType::kGptCacheAndroidIA),
        p("DC76DDA9-5AC1-491C-AF42-A82591580C0D", PartitionType::kGptDataAndroidIA),
        p("EBC597D0-2053-4B15-8B64-E0AAC75F4DB1", PartitionType::kGptPersistentAndroidIA),
        p("C5A0AEEC-13EA-11E5-A1B1-001E67CA0C3C", PartitionType::kGptVendorAndroidIA),
        p("BD59408B-4514-490D-BF12-9878D963F378", PartitionType::kGptConfigAndroidIA),
        p("8F68CC74-C5E5-48DA-BE91-A0C8C15E9C80", PartitionType::kGptFactoryAndroidIA),
        p("9FDAA6EF-4B3F-40D2-BA8D-BFF16BFB887B", PartitionType::kGptFactoryAltAndroidIA),
        p("767941D0-2085-11E3-AD3B-6CFDB94711E9", PartitionType::kGptFastbootOrTertiaryAndroidIA),
        p("AC6D7924-EB71-4DF8-B48D-E267B27148FF", PartitionType::kGptOEMAndroidIA),
        p("19A710A2-B3CA-11E4-B026-10604B889DCF", PartitionType::kGptAndroidMetaAndroid6Arm),
        p("193D1EA4-B3CA-11E4-B075-10604B889DCF", PartitionType::kGptAndroidEXTAndroid6Arm),
        p("7412F7D5-A156-4B13-81DC-867174929325", PartitionType::kGptBootONIE),
        p("D4E6E2CD-4469-46F3-B5CB-1BFF57AFC149", PartitionType::kGptConfigONIE),
        p("9E1A2D38-C612-4316-AA26-8B49521E5A8B", PartitionType::kGptPRePBootPowerPC),
        p("BC13C2FF-59E6-4262-A352-B275FD6F7172", PartitionType::kGptSharedBootloaderConfigurationFreedesktop),
        p("734E5AFE-F61A-11E6-BC64-92361F002671", PartitionType::kGptBasicDataPartitionAtariTOS),
        p("8C8F8EFF-AC95-4770-814A-21994F2DBC8F", PartitionType::kGptEncryptedDataPartitionVeraCrypt),
        p("90B6FF38-B98F-4358-A21F-48F35B4A8AD3", PartitionType::kGptArcaOSType1OS2),
        p("7C5222BD-8F5D-4087-9C00-BF9843C7B58C", PartitionType::kGptSPDKBlockDeviceSPDK),
        p("4778ED65-BF42-45FA-9C5B-287A1DC4AAB1", PartitionType::kGptBareBoxStateBareboxBootloader),
        p("3DE21764-95BD-54BD-A5C3-4ABE786F38A8", PartitionType::kGptUBootEnvironmentUBootBootloader),
        p("B6FA30DA-92D2-4A9A-96F1-871EC6486200", PartitionType::kGptStatusSoftRAID),
        p("2E313465-19B9-463F-8126-8A7993773801", PartitionType::kGptScratchSoftRAID),
        p("FA709C7E-65B1-4593-BFD5-E71D61DE9B02", PartitionType::kGptVolumeSoftRAID),
        p("BBBA6DF5-F46F-4A89-8F59-8765B2727503", PartitionType::kGptCacheSoftRAID),
        p("FE8A2634-5E2E-46BA-99E3-3A192091A350", PartitionType::kGptBootloaderFuchsiaStandard),
        p("D9FD4535-106C-4CEC-8D37-DFC020CA87CB", PartitionType::kGptDurablemutableencryptedsystemdataFuchsiaStandard),
        p("A409E16B-78AA-4ACC-995C-302352621A41", PartitionType::kGptDurablemutablebootloaderdataFuchsiaStandard),
        p("F95D940E-CABA-4578-9B93-BB6C90F29D3E", PartitionType::kGptFactoryProvisionedreadOnlysystemdataFuchsiaStandard),
        p("10B8DBAA-D2BF-42A9-98C6-A7C5DB3701E7", PartitionType::kGptFactoryProvisionedreadOnlybootloaderdataFuchsiaStandard),
        p("49FD7CB8-DF15-4E73-B9D9-992070127F0F", PartitionType::kGptFuchsiaVolumeManagerFuchsiaStandard),
        p("421A8BFC-85D9-4D85-ACDA-B64EEC0133E9", PartitionType::kGptVerifiedbootmetadataFuchsiaStandard),
        p("9B37FFF6-2E58-466A-983A-F7926D0B04E0", PartitionType::kGptZirconbootimageFuchsiaStandard),
        p("C12A7328-F81F-11D2-BA4B-00A0C93EC93B", PartitionType::kGptFuchsiaEspFuchsiaLegacy),
        p("606B000B-B7C7-4653-A7D5-B737332C899D", PartitionType::kGptFuchsiaSystemFuchsiaLegacy),
        p("08185F0C-892D-428A-A789-DBEEC8F55E6A", PartitionType::kGptFuchsiaDataFuchsiaLegacy),
        p("48435546-4953-2041-494E-5354414C4C52", PartitionType::kGptFuchsiaInstallFuchsiaLegacy),
        p("2967380E-134C-4CBB-B6DA-17E7CE1CA45D", PartitionType::kGptFuchsiaBlobFuchsiaLegacy),
        p("41D0E340-57E3-954E-8C1E-17ECAC44CFF5", PartitionType::kGptFuchsiaFvmFuchsiaLegacy),
        p("DE30CC86-1F4A-4A31-93C4-66F147D33E05", PartitionType::kGptZirconbootimageSlotAFuchsiaLegacy),
        p("23CC04DF-C278-4CE7-8471-897D1A4BCDF7", PartitionType::kGptZirconbootimageSlotBFuchsiaLegacy),
        p("A0E5CF57-2DEF-46BE-A80C-A2067C37CD49", PartitionType::kGptZirconbootimageSlotRFuchsiaLegacy),
        p("4E5E989E-4C86-11E8-A15B-480FCF35F8E6", PartitionType::kGptSysConfigFuchsiaLegacy),
        p("5A3A90BE-4C86-11E8-A15B-480FCF35F8E6", PartitionType::kGptFactoryConfigFuchsiaLegacy),
        p("5ECE94FE-4C86-11E8-A15B-480FCF35F8E6", PartitionType::kGptBootloaderFuchsiaLegacy),
        p("8B94D043-30BE-4871-9DFA-D69556E8C1F3", PartitionType::kGptGuidTestFuchsiaLegacy),
        p("A13B4D9A-EC5F-11E8-97D8-6C3BE52705BF", PartitionType::kGptVerifiedbootmetadataSlotAFuchsiaLegacy),
        p("A288ABF2-EC5F-11E8-97D8-6C3BE52705BF", PartitionType::kGptVerifiedbootmetadataSlotBFuchsiaLegacy),
        p("6A2460C3-CD11-4E8B-80A8-12CCE268ED0A", PartitionType::kGptVerifiedbootmetadataSlotRFuchsiaLegacy),
        p("1D75395D-F2C6-476B-A8B7-45CC1C97B476", PartitionType::kGptMiscFuchsiaLegacy),
        p("900B0FC5-90CD-4D4F-84F9-9F8ED579DB88", PartitionType::kGptEmmcBoot1FuchsiaLegacy),
        p("B2B2E8D1-7C10-4EBC-A2D0-4614568260AD", PartitionType::kGptEmmcBoot2FuchsiaLegacy),
    };

    if (datas.contains(guid))
        return datas.value(guid);
    return PartitionType::kPartitionTypeNotFound;
}

DeviceError Utils::castFromGError(const GError *const err)
{
    if (!err)
        return DeviceError::kNoError;

    const char *errDomain = g_quark_to_string(err->domain);
    if (strcmp(errDomain, UDISKS_ERR_DOMAIN) == 0) {
        return static_cast<DeviceError>(err->code + UDISKS_ERR_START);
    } else if (strcmp(errDomain, GIO_ERR_DOMAIN) == 0) {
        return static_cast<DeviceError>(err->code + GIO_ERR_START);
    } else if (strcmp(errDomain, GDBUS_ERR_DOMAIN) == 0) {
        return static_cast<DeviceError>(err->code + GDBUS_ERR_START);
    } else {
        qDebug() << "unhandled error: quark: " << g_quark_to_string(err->domain) << ", msg: " << err->message << ", code: " << err->code;
        return DeviceError::kUnhandledError;
    }
}

DeviceError Utils::castFromJobOperation(const QString &op)
{
#ifdef p
#    undef p
#endif
#define p(k, v) std::pair<QString, DeviceError>(k, DeviceError::v)
    const static QMap<QString, DeviceError> datas = {
        p("ata-smart-selftest", kUDisksBusySMARTSelfTesting),
        p("drive-eject", kUDisksBusyDriveEjecting),
        p("encrypted-unlock", kUDisksBusyEncryptedUnlocking),
        p("encrypted-lock", kUDisksBusyEncryptedLocking),
        p("encrypted-modify", kUDisksBusyEncryptedModifying),
        p("encrypted-resize", kUDisksBusyEncryptedResizing),
        p("swapspace-start", kUDisksBusySwapSpaceStarting),
        p("swapspace-stop", kUDisksBusySwapSpaceStoping),
        p("swapspace-modify", kUDisksBusySwpaSpaceModifying),
        p("filesystem-mount", kUDisksBusyFileSystemMounting),
        p("filesystem-unmount", kUDisksBusyFileSystemUnmounting),
        p("filesystem-modify", kUDisksBusyFileSystemModifying),
        p("filesystem-resize", kUDisksBusyFileSystemResizing),
        p("format-erase", kUDisksBusyFormatErasing),
        p("format-mkfs", kUDisksBusyFormatMkfsing),
        p("loop-setup", kUDisksBusyLoopSetuping),
        p("partition-modify", kUDisksBusyPartitionModifying),
        p("partition-delete", kUDisksBusyPartitionDeleting),
        p("partition-create", kUDisksBusyPartitionCreating),
        p("cleanup", kUDisksBusyCleanuping),
        p("ata-secure-erase", kUDisksBusyATASecureErasing),
        p("ata-enhanced-secure-erase", kUDisksBusyATAEnhancedSecureErasing),
        p("md-raid-stop", kUDisksBusyMdRaidStarting),
        p("md-raid-start", kUDisksBusyMdRaidStoping),
        p("md-raid-fault-device", kUDisksBusyMdRaidFaultingDevice),
        p("md-raid-remove-device", kUDisksBusyMdRaidRemovingDevice),
        p("md-raid-create", kUDisksBusyMdRaidCreating),
    };
    return datas.value(op, DeviceError::kUnhandledError);
}

QString Utils::getNameByProperty(Property property)
{
#ifdef rp
#    undef rp
#endif
#define rp(v, k) std::pair<Property, QString>(k, v)
    const static QMap<Property, QString> datas = {
        rp("UserspaceMountOptions", Property::kBlockUserspaceMountOptions),
        rp("Configuration", Property::kBlockConfiguration),
        rp("CryptoBackingDevice", Property::kBlockCryptoBackingDevice),
        rp("Device", Property::kBlockDevice),
        rp("Drive", Property::kBlockDrive),
        rp("IdLabel", Property::kBlockIDLabel),
        rp("IdType", Property::kBlockIDType),
        rp("IdUsage", Property::kBlockIDUsage),
        rp("IdUUID", Property::kBlockIDUUID),
        rp("IdVersion", Property::kBlockIDVersion),
        rp("DeviceNumber", Property::kBlockDeviceNumber),
        rp("PreferredDevice", Property::kBlockPreferredDevice),
        rp("Id", Property::kBlockID),
        rp("Size", Property::kBlockSize),
        rp("ReadOnly", Property::kBlockReadOnly),
        rp("Symlinks", Property::kBlockSymlinks),
        rp("HintPartitionable", Property::kBlockHintPartitionable),
        rp("HintSystem", Property::kBlockHintSystem),
        rp("HintIgnore", Property::kBlockHintIgnore),
        rp("HintAuto", Property::kBlockHintAuto),
        rp("HintName", Property::kBlockHintName),
        rp("HintIconName", Property::kBlockHintIconName),
        rp("HintSymbolicIconName", Property::kBlockHintSymbolicIconName),
        rp("MdRaid", Property::kBlockMdRaid),
        rp("MdRaidMember", Property::kBlockMdRaidMember),
        rp("ConnectionBus", Property::kDriveConnectionBus),
        rp("Removable", Property::kDriveRemovable),
        rp("Ejectable", Property::kDriveEjectable),
        rp("Seat", Property::kDriveSeat),
        rp("Media", Property::kDriveMedia),
        rp("MediaCompatibility", Property::kDriveMediaCompatibility),
        rp("MediaRemovable", Property::kDriveMediaRemovable),
        rp("MediaAvailable", Property::kDriveMediaAvailable),
        rp("MediaChangeDetected", Property::kDriveMediaChangeDetected),
        rp("TimeDetected", Property::kDriveTimeDetected),
        rp("TimeMediaDetected", Property::kDriveTimeMediaDetected),
        rp("Size", Property::kDriveSize),
        rp("Optical", Property::kDriveOptical),
        rp("OpticalBlank", Property::kDriveOpticalBlank),
        rp("OpticalNumTracks", Property::kDriveOpticalNumTracks),
        rp("OpticalNumAudioTracks", Property::kDriveOpticalNumAudioTracks),
        rp("OpticalNumDataTracks", Property::kDriveOpticalNumDataTracks),
        rp("OpticalNumSessions", Property::kDriveOpticalNumSessions),
        rp("Model", Property::kDriveModel),
        rp("Revision", Property::kDriveRevision),
        rp("RotationRate", Property::kDriveRotationRate),
        rp("Serial", Property::kDriveSerial),
        rp("Vender", Property::kDriveVender),
        rp("WWN", Property::kDriveWWN),
        rp("SortKey", Property::kDriveSortKey),
        rp("Configuration", Property::kDriveConfiguration),
        rp("ID", Property::kDriveID),
        rp("CanPowerOff", Property::kDriveCanPowerOff),
        rp("SiblingID", Property::kDriveSiblingID),
        rp("MountPoints", Property::kFileSystemMountPoint),
        rp("Number", Property::kPartitionNumber),
        rp("Type", Property::kPartitionType),
        rp("Offset", Property::kPartitionOffset),
        rp("Size", Property::kPartitionSize),
        rp("Flags", Property::kPartitionFlags),
        rp("Name", Property::kPartitionName),
        rp("UUID", Property::kPartitionUUID),
        rp("Table", Property::kPartitionTable),
        rp("IsContainer", Property::kPartitionIsContainer),
        rp("IsContained", Property::kPartitionIsContained),
        rp("ChildConfiguration", Property::kEncryptedChildConfiguration),
        rp("CleartextDevice", Property::kEncryptedCleartextDevice),
        rp("HintEncryptionType", Property::kEncryptedHintEncryptionType),
        rp("MetadataSize", Property::kEncryptedMetadataSize),
    };
    return datas.value(property, "");
}

QString Utils::errorMessage(DeviceError err)
{
#ifdef p
#    undef p
#endif
#define p(k, v) std::pair<DeviceError, QString>(DeviceError::k, v)
    static const QMap<DeviceError, QString> errors {
        p(kNoError, "No error"),
        p(kUDisksErrorFailed, "Failed"),
        p(kUDisksErrorCancelled, "Cancelled"),
        p(kUDisksErrorAlreadyCancelled, "Already cancelled"),
        p(kUDisksErrorNotAuthorized, "Not authorized"),
        p(kUDisksErrorNotAuthorizedCanObtain, "Not authorized can obtain"),
        p(kUDisksErrorNotAuthorizedDismissed, "Not authorized dismissed"),
        p(kUDisksErrorAlreadyMounted, "Device is already mounted"),
        p(kUDisksErrorNotMounted, "Device is not mounted"),
        p(kUDisksErrorOptionNotPermitted, "Option is not permitted"),
        p(kUDisksErrorMountedByOtherUser, "Device is mounted by other user"),
        p(kUDisksErrorAlreadyUnmounting, "Device is already unmounted"),
        p(kUDisksErrorNotSupproted, "Not supported operation"),
        p(kUDisksErrorTimedOut, "Operation time out"),
        p(kUDisksErrorWouldWakeup, "Operation would wakeup"),
        p(kUDisksErrorDeviceBusy, "Device is busy"),
        p(kUDisksErrorScsiDaemonTransportFailed, "SCSI daemon transport failed"),
        p(kUDisksErrorScsiHostNotFound, "SCSI host not found"),
        p(kUDisksErrorScsiIDMB, "SCSI IDMB"),
        p(kUDisksErrorScsiLoginFailed, "SCSI login failed"),
        p(kUDisksErrorScsiLoginAuthFailed, "SCSI login auth failed"),
        p(kUDisksErrorScsiLoginFatal, "SCSI login fatal"),
        p(kUDisksErrorScsiLogoutFailed, "SCSI logout failed"),
        p(kUDisksErrorScsiNoFirmware, "SCSI no firmware found"),
        p(kUDisksErrorScsiNoObjectsFound, "SCSI no objects found"),
        p(kUDisksErrorScsiNotConnected, "SCSI not connected"),
        p(kUDisksErrorScsiTransportFailed, "SCSI transport failed"),
        p(kUDisksErrorScsiUnknownDiscoveryType, "SCSI unknown discovery type"),

        // http://storaged.org/doc/udisks2-api/latest/gdbus-org.freedesktop.UDisks2.Job.html#gdbus-property-org-freedesktop-UDisks2-Job.Operation
        p(kUDisksBusySMARTSelfTesting, "SMART self-test operation"),
        p(kUDisksBusyDriveEjecting, "Ejecting the medium from a drive"),
        p(kUDisksBusyEncryptedUnlocking, "Unlocking encrypted device"),
        p(kUDisksBusyEncryptedLocking, "Locking encrypted device"),
        p(kUDisksBusyEncryptedModifying, "Modifying encrypted device"),
        p(kUDisksBusyEncryptedResizing, "Resizing encrypted device"),
        p(kUDisksBusySwapSpaceStarting, "Starting swapspace"),
        p(kUDisksBusySwapSpaceStoping, "Stopping swapspace"),
        p(kUDisksBusySwpaSpaceModifying, "Modifying swapspace"),
        p(kUDisksBusyFileSystemUnmounting, "Unmounting a filesystem"),
        p(kUDisksBusyFileSystemMounting, "Mounting a filesystem"),
        p(kUDisksBusyFileSystemModifying, "Modifying a filesystem"),
        p(kUDisksBusyFileSystemResizing, "Resizing a filesystem"),
        p(kUDisksBusyFormatErasing, "Erasing a device"),
        p(kUDisksBusyFormatMkfsing, "Creating a filesystem"),
        p(kUDisksBusyLoopSetuping, "Setting up a loop device"),
        p(kUDisksBusyPartitionModifying, "Modifying a partition"),
        p(kUDisksBusyPartitionDeleting, "Deleting a partition"),
        p(kUDisksBusyPartitionCreating, "Creating a partition"),
        p(kUDisksBusyCleanuping, "Cleaning up devices that were removed without being properly unmounted or shut down"),
        p(kUDisksBusyATASecureErasing, "ATA Secure Erase"),
        p(kUDisksBusyATAEnhancedSecureErasing, "ATA Enhanced Secure Erase"),
        p(kUDisksBusyMdRaidStarting, "Stopping a RAID Array"),
        p(kUDisksBusyMdRaidStoping, "Starting a RAID Array"),
        p(kUDisksBusyMdRaidFaultingDevice, "Marking device in RAID Array as faulty"),
        p(kUDisksBusyMdRaidRemovingDevice, "	Removing device from RAID Array"),
        p(kUDisksBusyMdRaidCreating, "Create a RAID Array"),

        p(kGIOErrorFailed, "GIOErrorFailed"),
        p(kGIOErrorNotFound, "GIOErrorNotFound"),
        p(kGIOErrorExists, "GIOErrorExists"),
        p(kGIOErrorIsDirectory, "GIOErrorIsDirectory"),
        p(kGIOErrorNotDirertory, "GIOErrorNotDirertory"),
        p(kGIOErrorNotEmpty, "GIOErrorNotEmpty"),
        p(kGIOErrorNotRegularFile, "GIOErrorNotRegularFile"),
        p(kGIOErrorNotSymbolicLink, "GIOErrorNotSymbolicLink"),
        p(kGIOErrorNotMountableFile, "GIOErrorNotMountableFile"),
        p(kGIOErrorFilenameTooLong, "GIOErrorFilenameTooLong"),
        p(kGIOErrorInvalidFilename, "GIOErrorInvalidFilename"),
        p(kGIOErrorTooManyLinks, "GIOErrorTooManyLinks"),
        p(kGIOErrorNoSpace, "GIOErrorNoSpace"),
        p(kGIOErrorInvalidArgument, "GIOErrorInvalidArgument"),
        p(kGIOErrorPermissionDenied, "GIOErrorPermissionDenied"),
        p(kGIOErrorNotSupported, "GIOErrorNotSupported"),
        p(kGIOErrorNotMounted, "GIOErrorNotMounted"),
        p(kGIOErrorAlreadyMounted, "GIOErrorAlreadyMounted"),
        p(kGIOErrorClosed, "GIOErrorClosed"),
        p(kGIOErrorCancelled, "GIOErrorCancelled"),
        p(kGIOErrorPending, "GIOErrorPending"),
        p(kGIOErrorReadONly, "GIOErrorReadONly"),
        p(kGIOErrorCantCreateBackup, "GIOErrorCantCreateBackup"),
        p(kGIOErrorWrongETAG, "GIOErrorWrongETAG"),
        p(kGIOErrorTimedOut, "GIOErrorTimedOut"),
        p(kGIOErrorWouldRecurse, "GIOErrorWouldRecurse"),
        p(kGIOErrorBusy, "GIOErrorBusy"),
        p(kGIOErrorWouldBlock, "GIOErrorWouldBlock"),
        p(kGIOErrorHostNotFound, "GIOErrorHostNotFound"),
        p(kGIOErrorWouldMerge, "GIOErrorWouldMerge"),
        p(kGIOErrorFailedHandled, "GIOErrorFailedHandled"),
        p(kGIOErrorTooManyOpenFiles, "GIOErrorTooManyOpenFiles"),
        p(kGIOErrorNotInitilized, "GIOErrorNotInitilized"),
        p(kGIOErrorAddressInUse, "GIOErrorAddressInUse"),
        p(kGIOErrorPartialInput, "GIOErrorPartialInput"),
        p(kGIOErrorInvalidData, "GIOErrorInvalidData"),
        p(kGIOErrorDBusError, "GIOErrorDBusError"),
        p(kGIOErrorHostUnreachable, "GIOErrorHostUnreachable"),
        p(kGIOErrorNetworkUnreachable, "GIOErrorNetworkUnreachable"),
        p(kGIOErrorConnectionRefused, "GIOErrorConnectionRefused"),
        p(kGIOErrorProxyFailed, "GIOErrorProxyFailed"),
        p(kGIOErrorProxyAuthFailed, "GIOErrorProxyAuthFailed"),
        p(kGIOErrorProxyNeedAuth, "GIOErrorProxyNeedAuth"),
        p(kGIOErrorProxyNotAllowed, "GIOErrorProxyNotAllowed"),
        p(kGIOErrorBrokenPipe, "GIOErrorBrokenPipe"),
        p(kGIOErrorConnectionClosed, "GIOErrorConnectionClosed"),
        p(kGIOErrorNotConnected, "GIOErrorNotConnected"),
        p(kGIOErrorMessageTooLarge, "GIOErrorMessageTooLarge"),

        p(kGDBusErrorFailed, "GDBusErrorFailed"),
        p(kGDBusErrorNoMemory, "GDBusErrorNoMemory"),
        p(kGDBusErrorServiceUnknown, "GDBusErrorServiceUnknown"),
        p(kGDBusErrorNameHasNoOwner, "GDBusErrorNameHasNoOwner"),
        p(kGDBusErrorNoReply, "GDBusErrorNoReply"),
        p(kGDBusErrorIOError, "GDBusErrorIOError"),
        p(kGDBusErrorBadAddress, "GDBusErrorBadAddress"),
        p(kGDBusErrorNotSupported, "GDBusErrorNotSupported"),
        p(kGDBusErrorLimitsExceeded, "GDBusErrorLimitsExceeded"),
        p(kGDBusErrorAccessDenied, "GDBusErrorAccessDenied"),
        p(kGDBusErrorAuthFailed, "GDBusErrorAuthFailed"),
        p(kGDBusErrorNoServer, "GDBusErrorNoServer"),
        p(kGDBusErrorTimeout, "GDBusErrorTimeout"),
        p(kGDBusErrorNoNetwork, "GDBusErrorNoNetwork"),
        p(kGDBusErrorAddressInUse, "GDBusErrorAddressInUse"),
        p(kGDBusErrorDisconnected, "GDBusErrorDisconnected"),
        p(kGDBusErrorInvalidArgs, "GDBusErrorInvalidArgs"),
        p(kGDBusErrorFileNotFound, "GDBusErrorFileNotFound"),
        p(kGDBusErrorFileExists, "GDBusErrorFileExists"),
        p(kGDBusErrorUnknownMethod, "GDBusErrorUnknownMethod"),
        p(kGDBusErrorTimedOut, "GDBusErrorTimedOut"),
        p(kGDBusErrorMatchRuleNotFound, "GDBusErrorMatchRuleNotFound"),
        p(kGDBusErrorMatchRuleInvalid, "GDBusErrorMatchRuleInvalid"),
        p(kGDBusErrorSpawnExecFailed, "GDBusErrorSpawnExecFailed"),
        p(kGDBusErrorSpawnForkFailed, "GDBusErrorSpawnForkFailed"),
        p(kGDBusErrorSpawnChildExited, "GDBusErrorSpawnChildExited"),
        p(kGDBusErrorSpawnChildSignaled, "GDBusErrorSpawnChildSignaled"),
        p(kGDBusErrorSpawnFailed, "GDBusErrorSpawnFailed"),
        p(kGDBusErrorSpawnSetupFailed, "GDBusErrorSpawnSetupFailed"),
        p(kGDBusErrorSpawnConfigInvalid, "GDBusErrorSpawnConfigInvalid"),
        p(kGDBusErrorSpawnServiceInvalid, "GDBusErrorSpawnServiceInvalid"),
        p(kGDBusErrorSpawnServiceNotFound, "GDBusErrorSpawnServiceNotFound"),
        p(kGDBusErrorSpawnPermissionsInvalid, "GDBusErrorSpawnPermissionsInvalid"),
        p(kGDBusErrorSpawnFileInvalid, "GDBusErrorSpawnFileInvalid"),
        p(kGDBusErrorSpawnNoMemory, "GDBusErrorSpawnNoMemory"),
        p(kGDBusErrorUnixProcessIdUnkown, "GDBusErrorUnixProcessIdUnkown"),
        p(kGDBusErrorInvalidSignature, "GDBusErrorInvalidSignature"),
        p(kGDBusErrorInvalidFileContent, "GDBusErrorInvalidFileContent"),
        p(kGDBusErrorSeLinuxSecurityContextUnknown, "GDBusErrorSeLinuxSecurityContextUnknown"),
        p(kGDBusErrorADTAuditDataUnknown, "GDBusErrorADTAuditDataUnknown"),
        p(kGDBusErrorObjectPathInUse, "GDBusErrorObjectPathInUse"),
        p(kGDBusErrorUnknownObject, "GDBusErrorUnknownObject"),
        p(kGDBusErrorUnknownInterface, "GDBusErrorUnknownInterface"),
        p(kGDBusErrorUnknownProperty, "GDBusErrorUnknownProperty"),
        p(kGDBusErrorPropertyReadOnly, "GDBusErrorPropertyReadOnly"),

        p(kUserErrorNotMountable, "Device is not mountable"),
        p(kUserErrorNotEjectable, "Device is not ejectable"),
        p(kUserErrorNoDriver, "Device do not have a drive"),
        p(kUserErrorNotEncryptable, "Device is not encryptable"),
        p(kUserErrorNoPartition, "Device do not have a partition"),
        p(kUserErrorNoBlock, "Device do not have a block"),
        p(kUserErrorNetworkWrongPasswd, "Wrong username or password"),
        p(kUserErrorNetworkAnonymousNotAllowed, "Anonymous login is not allowed"),
        p(kUserErrorTimedOut, "Operation timeout"),
        p(kUserErrorAlreadyMounted, "Device is already mounted"),
        p(kUserErrorNotMounted, "Device is not mounted"),
        p(kUserErrorFailed, "Operation failed"),

        p(kUnhandledError, "Unhandled error"),
    };
    return errors.value(err);
}

QString Utils::errorMessage(MonitorError err)
{
    static const QMap<MonitorError, QString> errors {
        std::pair<MonitorError, QString>(MonitorError::kNoError, "No error"),
        std::pair<MonitorError, QString>(MonitorError::kMonitorNotRegister, "Not registered monitor"),
        std::pair<MonitorError, QString>(MonitorError::kMonitorAlreadyRegistered, "Monitor is already registered"),
    };
    return errors.value(err);
}

QString Utils::gcharToQString(char *tmp)
{
    if (!tmp)
        return QString();
    QString ret(tmp);
    g_free(tmp);
    tmp = nullptr;
    return ret;
}

QStringList Utils::gcharvToQStringList(char **tmp)
{
    QStringList ret;
    int next = 0;
    while (tmp && tmp[next]) {
        ret << QString(tmp[next]);
        next += 1;
    }
    g_strfreev(tmp);
    tmp = nullptr;
    return ret;
}

QString Utils::currentUser()
{
    auto userInfo = getpwuid(getuid());
    if (userInfo)
        return userInfo->pw_name;
    return "";
}
