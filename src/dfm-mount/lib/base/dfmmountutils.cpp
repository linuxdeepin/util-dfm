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

#include "base/dfmmountutils.h"

#include <QDebug>

#include <glib.h>

DFM_MOUNT_USE_NS

// use G_VARIANT_TYPE_* gets alot compile warning
#define GVAR_TYPE_BOOL              reinterpret_cast<const GVariantType *>("b")
#define GVAR_TYPE_BYTE              reinterpret_cast<const GVariantType *>("y")
#define GVAR_TYPE_INT16             reinterpret_cast<const GVariantType *>("n")
#define GVAR_TYPE_UINT16            reinterpret_cast<const GVariantType *>("q")
#define GVAR_TYPE_INT32             reinterpret_cast<const GVariantType *>("i")
#define GVAR_TYPE_UINT32            reinterpret_cast<const GVariantType *>("u")
#define GVAR_TYPE_INT64             reinterpret_cast<const GVariantType *>("x")
#define GVAR_TYPE_UINT64            reinterpret_cast<const GVariantType *>("t")
#define GVAR_TYPE_DOUBLE            reinterpret_cast<const GVariantType *>("d")
#define GVAR_TYPE_STRING            reinterpret_cast<const GVariantType *>("s")
#define GVAR_TYPE_STRING_ARR        reinterpret_cast<const GVariantType *>("as")
#define GVAR_TYPE_OBJECTPATH        reinterpret_cast<const GVariantType *>("o")
#define GVAR_TYPE_OBJECTPATH_ARR    reinterpret_cast<const GVariantType *>("ao")
#define GVAR_TYPE_VARIANT           reinterpret_cast<const GVariantType *>("v")
#define GVAR_TYPE_BYTESTRING        reinterpret_cast<const GVariantType *>("ay")
#define GVAR_TYPE_BYTESTRING_ARR    reinterpret_cast<const GVariantType *>("aay")
#define GVAR_TYPE_VARDICT           reinterpret_cast<const GVariantType *>("a{sv}")
#define GVAR_TYPE_ARR               reinterpret_cast<const GVariantType *>("a*")

QVariant Utils::castFromGVariant(GVariant *val) {
    auto isType = [val](const GVariantType * type) {
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
        g_variant_get (val, "a{sv}", &iter);
        char *key = nullptr;
        GVariant *item = nullptr;
        while (g_variant_iter_next(iter, "{&sv}", &key, &item))
            ret.insert(QString(key), castFromGVariant(item));
        g_variant_iter_free(iter);
        return ret;
    }
    if (isType(GVAR_TYPE_ARR)) {
        QList<QVariant> lst;
        GVariantIter *iter = nullptr;
        g_variant_get(val, "av", &iter);
        GVariant *item = nullptr;
        while (g_variant_iter_loop(iter, "v", &item))
            lst << castFromGVariant(item);
        g_variant_iter_free(iter);
        return lst;
    }
    qDebug() << g_variant_classify(val) << "cannot be parsed";
    return QVariant();
}

GVariant *Utils::castFromQVariant(const QVariant &val) {
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

GVariant *Utils::castFromQVariantMap(const QVariantMap &val) {
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));
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
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE ("as"));
    if (!builder) {
        qWarning() << "cannot allocate a gvariantbuilder";
        return nullptr;
    }

    for (const auto &s: val) {
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
    GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE ("av"));
    if (!builder) {
        qWarning() << "cannot allocate a gvariantbuilder";
        return nullptr;
    }

    for (auto valItem: val) {
        GVariant *item = castFromQVariant(valItem);
        if (item)
            g_variant_builder_add(builder, "v", item);
    }

    GVariant *ret = g_variant_builder_end(builder);
    g_variant_builder_unref(builder);
    return ret;
}

#define StringPropertyItem(key, val)    std::pair<QString, Property>(key, val)
#define PropertyStringItem(val, key)    std::pair<Property, QString>(key, val)
Property Utils::getPropertyByName(const QString &name) {
    const static QMap<QString, Property> datas = {
        StringPropertyItem("Configuration",           Property::BlockConfiguration),
        StringPropertyItem("CryptoBackingDevice",     Property::BlockCryptoBackingDevice),
        StringPropertyItem("Device",                  Property::BlockDevice),
        StringPropertyItem("Drive",                   Property::BlockDrive),
        StringPropertyItem("IdLabel",                 Property::BlockIDLabel),
        StringPropertyItem("IdType",                  Property::BlockIDType),
        StringPropertyItem("IdUsage",                 Property::BlockIDUsage),
        StringPropertyItem("IdUUID",                  Property::BlockIDUUID),
        StringPropertyItem("IdVersion",               Property::BlockIDVersion),
        StringPropertyItem("DeviceNumber",            Property::BlockDeviceNumber),
        StringPropertyItem("PreferredDevice",         Property::BlockPreferredDevice),
        StringPropertyItem("Id",                      Property::BlockID),
        StringPropertyItem("Size",                    Property::BlockSize),
        StringPropertyItem("ReadOnly",                Property::BlockReadOnly),
        StringPropertyItem("Symlinks",                Property::BlockSymlinks),
        StringPropertyItem("HintPartitionable",       Property::BlockHintPartitionable),
        StringPropertyItem("HintSystem",              Property::BlockHintSystem),
        StringPropertyItem("HintIgnore",              Property::BlockHintIgnore),
        StringPropertyItem("HintAuto",                Property::BlockHintAuto),
        StringPropertyItem("HintName",                Property::BlockHintName),
        StringPropertyItem("HintIconName",            Property::BlockHintIconName),
        StringPropertyItem("HintSymbolicIconName",    Property::BlockHintSymbolicIconName),
        StringPropertyItem("MdRaid",                  Property::BlockMdRaid),
        StringPropertyItem("MdRaidMember",            Property::BlockMdRaidMember),
        StringPropertyItem("ConnectionBus",           Property::DriveConnectionBus),
        StringPropertyItem("Removable",               Property::DriveRemovable),
        StringPropertyItem("Ejectable",               Property::DriveEjectable),
        StringPropertyItem("Seat",                    Property::DriveSeat),
        StringPropertyItem("Media",                   Property::DriveMedia),
        StringPropertyItem("MediaCompatibility",      Property::DriveMediaCompatibility),
        StringPropertyItem("MediaRemovable",          Property::DriveMediaRemovable),
        StringPropertyItem("MediaAvailable",          Property::DriveMediaAvailable),
        StringPropertyItem("MediaChangeDetected",     Property::DriveMediaChangeDetected),
        StringPropertyItem("TimeDetected",            Property::DriveTimeDetected),
        StringPropertyItem("TimeMediaDetected",       Property::DriveTimeMediaDetected),
        StringPropertyItem("Size",                    Property::DriveSize),
        StringPropertyItem("Optical",                 Property::DriveOptical),
        StringPropertyItem("OpticalBlank",            Property::DriveOpticalBlank),
        StringPropertyItem("OpticalNumTracks",        Property::DriveOpticalNumTracks),
        StringPropertyItem("OpticalNumAudioTracks",   Property::DriveOpticalNumAudioTracks),
        StringPropertyItem("OpticalNumDataTracks",    Property::DriveOpticalNumDataTracks),
        StringPropertyItem("OpticalNumSessions",      Property::DriveOpticalNumSessions),
        StringPropertyItem("Model",                   Property::DriveModel),
        StringPropertyItem("Revision",                Property::DriveRevision),
        StringPropertyItem("RotationRate",            Property::DriveRotationRate),
        StringPropertyItem("Serial",                  Property::DriveSerial),
        StringPropertyItem("Vender",                  Property::DriveVender),
        StringPropertyItem("WWN",                     Property::DriveWWN),
        StringPropertyItem("SortKey",                 Property::DriveSortKey),
        StringPropertyItem("Configuration",           Property::DriveConfiguration),
        StringPropertyItem("ID",                      Property::DriveID),
        StringPropertyItem("CanPowerOff",             Property::DriveCanPowerOff),
        StringPropertyItem("SiblingID",               Property::DriveSiblingID),
        StringPropertyItem("MountPoints",             Property::FileSystemMountPoint),
        StringPropertyItem("Number",                  Property::PartitionNumber),
        StringPropertyItem("Type",                    Property::PartitionType),
        StringPropertyItem("Offset",                  Property::PartitionOffset),
        StringPropertyItem("Size",                    Property::PartitionSize),
        StringPropertyItem("Flags",                   Property::PartitionFlags),
        StringPropertyItem("Name",                    Property::PartitionName),
        StringPropertyItem("UUID",                    Property::PartitionUUID),
        StringPropertyItem("Table",                   Property::PartitionTable),
        StringPropertyItem("IsContainer",             Property::PartitionIsContainer),
        StringPropertyItem("IsContained",             Property::PartitionIsContained),
        StringPropertyItem("ChildConfiguration",      Property::EncryptedChildConfiguration),
        StringPropertyItem("CleartextDevice",         Property::EncryptedCleartextDevice),
        StringPropertyItem("HintEncryptionType",      Property::EncryptedHintEncryptionType),
        StringPropertyItem("MetadataSize",            Property::EncryptedMetadataSize),
    };
    return datas.value(name, Property::NotInit);
}

#define GuidAndTypeItem(key, val)       std::pair<QString, PartitionType>(key, val)
PartitionType Utils::getPartitionTypeByGuid(const QString &guid)
{
    static const QMap<QString, PartitionType> datas = {
        GuidAndTypeItem("00000000-0000-0000-0000-000000000000", PartitionType::GptUnusedEntryNA),
        GuidAndTypeItem("024DEE41-33E7-11D3-9D69-0008C781F39F", PartitionType::GptMBRPartitionSchemeNA),
        GuidAndTypeItem("C12A7328-F81F-11D2-BA4B-00A0C93EC93B", PartitionType::GptEFISystemPartitionNA),
        GuidAndTypeItem("21686148-6449-6E6F-744E-656564454649", PartitionType::GptBIOSBootPartitionNA),
        GuidAndTypeItem("D3BFE2DE-3DAF-11DF-BA40-E3A556D89593", PartitionType::GptIntelFastFlashPartitionNA),
        GuidAndTypeItem("F4019732-066E-4E12-8273-346C5641494F", PartitionType::GptSonyBootPartitionNA),
        GuidAndTypeItem("BFBFAFE7-A34F-448A-9A5B-6213EB736C22", PartitionType::GptLenovoBootPartitionNA),
        GuidAndTypeItem("E3C9E316-0B5C-4DB8-817D-F92DF00215AE", PartitionType::GptMicrosoftReservedPartitionWin),
        GuidAndTypeItem("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", PartitionType::GptBasicDataPartitionWin),
        GuidAndTypeItem("5808C8AA-7E8F-42E0-85D2-E1E90434CFB3", PartitionType::GptLogicalDiskManagerMetaDataPartitionWin),
        GuidAndTypeItem("AF9B60A0-1431-4F62-BC68-3311714A69AD", PartitionType::GptLogicalDiskManagerDataPartitionWin),
        GuidAndTypeItem("DE94BBA4-06D1-4D40-A16A-BFD50179D6AC", PartitionType::GptWindowsRecoveryEnvironmentWin),
        GuidAndTypeItem("37AFFC90-EF7D-4E96-91C3-2D7AE055B174", PartitionType::GptIBMGeneralParallelFileSystemPartitionWin),
        GuidAndTypeItem("E75CAF8F-F680-4CEE-AFA3-B001E56EFC2D", PartitionType::GptStorageSpacesPartitionWin),
        GuidAndTypeItem("558D43C5-A1AC-43C0-AAC8-D1472B2923D1", PartitionType::GptStorageReplicaPartitionWin),
        GuidAndTypeItem("75894C1E-3AEB-11D3-B7C1-7B03A0000000", PartitionType::GptDataPartitionHPUX),
        GuidAndTypeItem("E2A1E728-32E3-11D6-A682-7B03A0000000", PartitionType::GptServicePartitionHPUX),
        GuidAndTypeItem("0FC63DAF-8483-4772-8E79-3D69D8477DE4", PartitionType::GptLinuxFilesystemDataLinux),
        GuidAndTypeItem("A19D880F-05FC-4D3B-A006-743F0F84911E", PartitionType::GptRAIDPartitionLinux),
        GuidAndTypeItem("44479540-F297-41B2-9AF7-D131D5F0458A", PartitionType::GptRootPartitionX86Linux),
        GuidAndTypeItem("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709", PartitionType::GptRootPartitionX8664Linux),
        GuidAndTypeItem("69DAD710-2CE4-4E3C-B16C-21A1D49ABED3", PartitionType::GptRootPartitionArm32Linux),
        GuidAndTypeItem("B921B045-1DF0-41C3-AF44-4C6F280D3FAE", PartitionType::GptRootPartitionArm64Linux),
        GuidAndTypeItem("BC13C2FF-59E6-4262-A352-B275FD6F7172", PartitionType::GptBootPartitionLinux),
        GuidAndTypeItem("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F", PartitionType::GptSwapPartitionLinux),
        GuidAndTypeItem("E6D6D379-F507-44C2-A23C-238F2A3DF928", PartitionType::GptLogicalVolumeManagerPartitionLinux),
        GuidAndTypeItem("933AC7E1-2EB4-4F13-B844-0E14E2AEF915", PartitionType::GptHomePartitionLinux),
        GuidAndTypeItem("3B8F8425-20E0-4F3B-907F-1A25A76F98E8", PartitionType::GptServerDataPartitionLinux),
        GuidAndTypeItem("7FFEC5C9-2D00-49B7-8941-3EA10A5586B7", PartitionType::GptPlainDMCryptPartitionLinux),
        GuidAndTypeItem("CA7D7CCB-63ED-4C53-861C-1742536059CC", PartitionType::GptLUKSPartitionLinux),
        GuidAndTypeItem("8DA63339-0007-60C0-C436-083AC8230908", PartitionType::GptReservedLinux),
        GuidAndTypeItem("83BD6B9D-7F41-11DC-BE0B-001560B84F0F", PartitionType::GptBootPartitionFreeBSD),
        GuidAndTypeItem("516E7CB4-6ECF-11D6-8FF8-00022D09712B", PartitionType::GptBSDDisklabelPartitionFreeBSD),
        GuidAndTypeItem("516E7CB5-6ECF-11D6-8FF8-00022D09712B", PartitionType::GptSwapPartitionFreeBSD),
        GuidAndTypeItem("516E7CB6-6ECF-11D6-8FF8-00022D09712B", PartitionType::GptUnixFileSystemPartitionFreeBSD),
        GuidAndTypeItem("516E7CB8-6ECF-11D6-8FF8-00022D09712B", PartitionType::GptVinumVolumeManagerPartitionFreeBSD),
        GuidAndTypeItem("516E7CBA-6ECF-11D6-8FF8-00022D09712B", PartitionType::GptZFSPartitionFreeBSD),
        GuidAndTypeItem("74BA7DD9-A689-11E1-BD04-00E081286ACF", PartitionType::GptNandfsPartitionFreeBSD),
        GuidAndTypeItem("48465300-0000-11AA-AA11-00306543ECAC", PartitionType::GptHierarchialFileSystemPlusPartitionMacOS),
        GuidAndTypeItem("7C3457EF-0000-11AA-AA11-00306543ECAC", PartitionType::GptAppleAPFSContainerMacOS),
        GuidAndTypeItem("55465300-0000-11AA-AA11-00306543ECAC", PartitionType::GptAppleUFSContainerMacOS),
        GuidAndTypeItem("6A898CC3-1DD2-11B2-99A6-080020736631", PartitionType::GptZFSMacOS),
        GuidAndTypeItem("52414944-0000-11AA-AA11-00306543ECAC", PartitionType::GptAppleRAIDPartitionMacOS),
        GuidAndTypeItem("52414944-5F4F-11AA-AA11-00306543ECAC", PartitionType::GptAppleRAIDPartitionOfflineMacOS),
        GuidAndTypeItem("426F6F74-0000-11AA-AA11-00306543ECAC", PartitionType::GptAppleBootPartitionMacOS),
        GuidAndTypeItem("4C616265-6C00-11AA-AA11-00306543ECAC", PartitionType::GptAppleLabelMacOS),
        GuidAndTypeItem("5265636F-7665-11AA-AA11-00306543ECAC", PartitionType::GptAppleTVRecoveryPartitionMacOS),
        GuidAndTypeItem("53746F72-6167-11AA-AA11-00306543ECAC", PartitionType::GptAppleCoreStorageContainerMacOS),
        GuidAndTypeItem("69646961-6700-11AA-AA11-00306543ECAC", PartitionType::GptAppleAPFSPrebootPartitionMacOS),
        GuidAndTypeItem("52637672-7900-11AA-AA11-00306543ECAC", PartitionType::GptAppleAPFSRecoveryPartitionMacOS),
        GuidAndTypeItem("6A82CB45-1DD2-11B2-99A6-080020736631", PartitionType::GptBootPartitionSolaris),
        GuidAndTypeItem("6A85CF4D-1DD2-11B2-99A6-080020736631", PartitionType::GptRootPartitionSolaris),
        GuidAndTypeItem("6A87C46F-1DD2-11B2-99A6-080020736631", PartitionType::GptSwapPartitionSolaris),
        GuidAndTypeItem("6A8B642B-1DD2-11B2-99A6-080020736631", PartitionType::GptBackupPartitionSolaris),
        GuidAndTypeItem("6A898CC3-1DD2-11B2-99A6-080020736631", PartitionType::GptUsrPartitionSolaris),
        GuidAndTypeItem("6A8EF2E9-1DD2-11B2-99A6-080020736631", PartitionType::GptVarPartitionSolaris),
        GuidAndTypeItem("6A90BA39-1DD2-11B2-99A6-080020736631", PartitionType::GptHomePartitionSolaris),
        GuidAndTypeItem("6A9283A5-1DD2-11B2-99A6-080020736631", PartitionType::GptAlternateSectorSolaris),
        GuidAndTypeItem("6A945A3B-1DD2-11B2-99A6-080020736631", PartitionType::GptReservedPartitionSolaris),
        GuidAndTypeItem("6A9630D1-1DD2-11B2-99A6-080020736631", PartitionType::GptReservedPartitionSolaris),
        GuidAndTypeItem("6A980767-1DD2-11B2-99A6-080020736631", PartitionType::GptReservedPartitionSolaris),
        GuidAndTypeItem("6A96237F-1DD2-11B2-99A6-080020736631", PartitionType::GptReservedPartitionSolaris),
        GuidAndTypeItem("6A8D2AC7-1DD2-11B2-99A6-080020736631", PartitionType::GptReservedPartitionSolaris),
        GuidAndTypeItem("49F48D32-B10E-11DC-B99B-0019D1879648", PartitionType::GptSwapPartitionNetBSD),
        GuidAndTypeItem("49F48D5A-B10E-11DC-B99B-0019D1879648", PartitionType::GptFFSPartitionNetBSD),
        GuidAndTypeItem("49F48D82-B10E-11DC-B99B-0019D1879648", PartitionType::GptLFSPartitionNetBSD),
        GuidAndTypeItem("49F48DAA-B10E-11DC-B99B-0019D1879648", PartitionType::GptRAIDPartitionNetBSD),
        GuidAndTypeItem("2DB519C4-B10F-11DC-B99B-0019D1879648", PartitionType::GptConcatenatedPartitionNetBSD),
        GuidAndTypeItem("2DB519EC-B10F-11DC-B99B-0019D1879648", PartitionType::GptEncryptedPartitionNetBSD),
        GuidAndTypeItem("FE3A2A5D-4F32-41A7-B725-ACCC3285A309", PartitionType::GptKernelChromeOS),
        GuidAndTypeItem("3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC", PartitionType::GptRootfsChromeOS),
        GuidAndTypeItem("CAB6E88E-ABF3-4102-A07A-D4BB9BE3C1D3", PartitionType::GptFirmwareChromeOS),
        GuidAndTypeItem("2E0A753D-9E48-43B0-8337-B15192CB1B5E", PartitionType::GptFutureUseChromeOS),
        GuidAndTypeItem("09845860-705F-4BB5-B16C-8A8A099CAF52", PartitionType::GptMiniOSChromeOS),
        GuidAndTypeItem("3F0F8318-F146-4E6B-8222-C28C8F02E0D5", PartitionType::GptHibernateChromeOS),
        GuidAndTypeItem("5DFBF5F4-2848-4BAC-AA5E-0D9A20B745A6", PartitionType::GptUsrPartitionCoreOS),
        GuidAndTypeItem("3884DD41-8582-4404-B9A8-E9B84F2DF50E", PartitionType::GptResizableRootfsCoreOS),
        GuidAndTypeItem("C95DC21A-DF0E-4340-8D7B-26CBFA9A03E0", PartitionType::GptOEMCustomizationsCoreOS),
        GuidAndTypeItem("BE9067B9-EA49-4F15-B4F6-F36F8C9E1818", PartitionType::GptRootFilesystemOnRAIDCoreOS),
        GuidAndTypeItem("42465331-3BA3-10F1-802A-4861696B7521", PartitionType::GptHaikuBFSHaiku),
        GuidAndTypeItem("85D5E45E-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::GptBootPartitionMidnightBSD),
        GuidAndTypeItem("85D5E45A-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::GptDataPartitionMidnightBSD),
        GuidAndTypeItem("85D5E45B-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::GptSwapPartitionMidnightBSD),
        GuidAndTypeItem("0394EF8B-237E-11E1-B4B3-E89A8F7FC3A7", PartitionType::GptUnixFileSystemPartitionMidnightBSD),
        GuidAndTypeItem("85D5E45C-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::GptVinumVolumemanagerPartitionMidnightBSD),
        GuidAndTypeItem("85D5E45D-237C-11E1-B4B3-E89A8F7FC3A7", PartitionType::GptZFSPartitionMidnightBSD),
        GuidAndTypeItem("45B0969E-9B03-4F30-B4C6-B4B80CEFF106", PartitionType::GptJournalCeph),
        GuidAndTypeItem("45B0969E-9B03-4F30-B4C6-5EC00CEFF106", PartitionType::GptDmCryptJournalCeph),
        GuidAndTypeItem("4FBD7E29-9D25-41B8-AFD0-062C0CEFF05D", PartitionType::GptOSDCeph),
        GuidAndTypeItem("4FBD7E29-9D25-41B8-AFD0-5EC00CEFF05D", PartitionType::GptDmCryptOSDCeph),
        GuidAndTypeItem("89C57F98-2FE5-4DC0-89C1-F3AD0CEFF2BE", PartitionType::GptDiskinCreationCeph),
        GuidAndTypeItem("89C57F98-2FE5-4DC0-89C1-5EC00CEFF2BE", PartitionType::GptDmCryptDiskinCreationCeph),
        GuidAndTypeItem("CAFECAFE-9B03-4F30-B4C6-B4B80CEFF106", PartitionType::GptBlockCeph),
        GuidAndTypeItem("30CD0809-C2B2-499C-8879-2D6B78529876", PartitionType::GptBlockDBCeph),
        GuidAndTypeItem("5CE17FCE-4087-4169-B7FF-056CC58473F9", PartitionType::GptBlockWriteAheadlogCeph),
        GuidAndTypeItem("FB3AABF9-D25F-47CC-BF5E-721D1816496B", PartitionType::GptLockboxForDmCryptKeysCeph),
        GuidAndTypeItem("4FBD7E29-8AE0-4982-BF9D-5A8D867AF560", PartitionType::GptMultipathOSDCeph),
        GuidAndTypeItem("45B0969E-8AE0-4982-BF9D-5A8D867AF560", PartitionType::GptMultipathJournalCeph),
        GuidAndTypeItem("CAFECAFE-8AE0-4982-BF9D-5A8D867AF560", PartitionType::GptMultipathBlockCeph),
        GuidAndTypeItem("7F4A666A-16F3-47A2-8445-152EF4D03F6C", PartitionType::GptMultipathBlockCeph),
        GuidAndTypeItem("EC6D6385-E346-45DC-BE91-DA2A7C8B3261", PartitionType::GptMultipathBlockDBCeph),
        GuidAndTypeItem("01B41E1B-002A-453C-9F17-88793989FF8F", PartitionType::GptMultipathblockwriteAheadogCeph),
        GuidAndTypeItem("CAFECAFE-9B03-4F30-B4C6-5EC00CEFF106", PartitionType::GptDmCryptBlockCeph),
        GuidAndTypeItem("93B0052D-02D9-4D8A-A43B-33A3EE4DFBC3", PartitionType::GptDmCryptBlockDBCeph),
        GuidAndTypeItem("306E8683-4FE2-4330-B7C0-00A917C16966", PartitionType::GptDmCryptBlockWriteAheadlogCeph),
        GuidAndTypeItem("45B0969E-9B03-4F30-B4C6-35865CEFF106", PartitionType::GptDmCryptLUKSjournalCeph),
        GuidAndTypeItem("CAFECAFE-9B03-4F30-B4C6-35865CEFF106", PartitionType::GptDmCryptLUKSBlockCeph),
        GuidAndTypeItem("166418DA-C469-4022-ADF4-B30AFD37F176", PartitionType::GptDmCryptLUKSBlockDBCeph),
        GuidAndTypeItem("86A32090-3647-40B9-BBBD-38D8C573AA86", PartitionType::GptDmCryptLUKSBlockwriteAheadlogCeph),
        GuidAndTypeItem("4FBD7E29-9D25-41B8-AFD0-35865CEFF05D", PartitionType::GptDmCryptLUKSOSDCeph),
        GuidAndTypeItem("824CC7A0-36A8-11E3-890A-952519AD3F61", PartitionType::GptDataPartitionOpenBSD),
        GuidAndTypeItem("CEF5A9AD-73BC-4601-89F3-CDEEEEE321A1", PartitionType::GptPowerSafeFilesystemQNX),
        GuidAndTypeItem("C91818F9-8025-47AF-89D2-F030D7000C2C", PartitionType::GptPlan9PartitionPlan9),
        GuidAndTypeItem("9D275380-40AD-11DB-BF97-000C2911D1B8", PartitionType::GptVmkCoreVMwareESX),
        GuidAndTypeItem("AA31E02A-400F-11DB-9590-000C2911D1B8", PartitionType::GptVMFSFilesystemPartitionVMwareESX),
        GuidAndTypeItem("9198EFFC-31C0-11DB-8F78-000C2911D1B8", PartitionType::GptVMWareReservedVMwareESX),
        GuidAndTypeItem("2568845D-2332-4675-BC39-8FA5A4748D15", PartitionType::GptBootloaderAndroidIA),
        GuidAndTypeItem("114EAFFE-1552-4022-B26E-9B053604CF84", PartitionType::GptBootloader2AndroidIA),
        GuidAndTypeItem("49A4D17F-93A3-45C1-A0DE-F50B2EBE2599", PartitionType::GptBootAndroidIA),
        GuidAndTypeItem("4177C722-9E92-4AAB-8644-43502BFD5506", PartitionType::GptRecoveryAndroidIA),
        GuidAndTypeItem("EF32A33B-A409-486C-9141-9FFB711F6266", PartitionType::GptMiscAndroidIA),
        GuidAndTypeItem("20AC26BE-20B7-11E3-84C5-6CFDB94711E9", PartitionType::GptMetadataAndroidIA),
        GuidAndTypeItem("38F428E6-D326-425D-9140-6E0EA133647C", PartitionType::GptSystemAndroidIA),
        GuidAndTypeItem("A893EF21-E428-470A-9E55-0668FD91A2D9", PartitionType::GptCacheAndroidIA),
        GuidAndTypeItem("DC76DDA9-5AC1-491C-AF42-A82591580C0D", PartitionType::GptDataAndroidIA),
        GuidAndTypeItem("EBC597D0-2053-4B15-8B64-E0AAC75F4DB1", PartitionType::GptPersistentAndroidIA),
        GuidAndTypeItem("C5A0AEEC-13EA-11E5-A1B1-001E67CA0C3C", PartitionType::GptVendorAndroidIA),
        GuidAndTypeItem("BD59408B-4514-490D-BF12-9878D963F378", PartitionType::GptConfigAndroidIA),
        GuidAndTypeItem("8F68CC74-C5E5-48DA-BE91-A0C8C15E9C80", PartitionType::GptFactoryAndroidIA),
        GuidAndTypeItem("9FDAA6EF-4B3F-40D2-BA8D-BFF16BFB887B", PartitionType::GptFactoryAltAndroidIA),
        GuidAndTypeItem("767941D0-2085-11E3-AD3B-6CFDB94711E9", PartitionType::GptFastbootOrTertiaryAndroidIA),
        GuidAndTypeItem("AC6D7924-EB71-4DF8-B48D-E267B27148FF", PartitionType::GptOEMAndroidIA),
        GuidAndTypeItem("19A710A2-B3CA-11E4-B026-10604B889DCF", PartitionType::GptAndroidMetaAndroid6Arm),
        GuidAndTypeItem("193D1EA4-B3CA-11E4-B075-10604B889DCF", PartitionType::GptAndroidEXTAndroid6Arm),
        GuidAndTypeItem("7412F7D5-A156-4B13-81DC-867174929325", PartitionType::GptBootONIE),
        GuidAndTypeItem("D4E6E2CD-4469-46F3-B5CB-1BFF57AFC149", PartitionType::GptConfigONIE),
        GuidAndTypeItem("9E1A2D38-C612-4316-AA26-8B49521E5A8B", PartitionType::GptPRePBootPowerPC),
        GuidAndTypeItem("BC13C2FF-59E6-4262-A352-B275FD6F7172", PartitionType::GptSharedBootloaderConfigurationFreedesktop),
        GuidAndTypeItem("734E5AFE-F61A-11E6-BC64-92361F002671", PartitionType::GptBasicDataPartitionAtariTOS),
        GuidAndTypeItem("8C8F8EFF-AC95-4770-814A-21994F2DBC8F", PartitionType::GptEncryptedDataPartitionVeraCrypt),
        GuidAndTypeItem("90B6FF38-B98F-4358-A21F-48F35B4A8AD3", PartitionType::GptArcaOSType1OS2),
        GuidAndTypeItem("7C5222BD-8F5D-4087-9C00-BF9843C7B58C", PartitionType::GptSPDKBlockDeviceSPDK),
        GuidAndTypeItem("4778ED65-BF42-45FA-9C5B-287A1DC4AAB1", PartitionType::GptBareBoxStateBareboxBootloader),
        GuidAndTypeItem("3DE21764-95BD-54BD-A5C3-4ABE786F38A8", PartitionType::GptUBootEnvironmentUBootBootloader),
        GuidAndTypeItem("B6FA30DA-92D2-4A9A-96F1-871EC6486200", PartitionType::GptStatusSoftRAID),
        GuidAndTypeItem("2E313465-19B9-463F-8126-8A7993773801", PartitionType::GptScratchSoftRAID),
        GuidAndTypeItem("FA709C7E-65B1-4593-BFD5-E71D61DE9B02", PartitionType::GptVolumeSoftRAID),
        GuidAndTypeItem("BBBA6DF5-F46F-4A89-8F59-8765B2727503", PartitionType::GptCacheSoftRAID),
        GuidAndTypeItem("FE8A2634-5E2E-46BA-99E3-3A192091A350", PartitionType::GptBootloaderFuchsiaStandard),
        GuidAndTypeItem("D9FD4535-106C-4CEC-8D37-DFC020CA87CB", PartitionType::GptDurablemutableencryptedsystemdataFuchsiaStandard),
        GuidAndTypeItem("A409E16B-78AA-4ACC-995C-302352621A41", PartitionType::GptDurablemutablebootloaderdataFuchsiaStandard),
        GuidAndTypeItem("F95D940E-CABA-4578-9B93-BB6C90F29D3E", PartitionType::GptFactoryProvisionedreadOnlysystemdataFuchsiaStandard),
        GuidAndTypeItem("10B8DBAA-D2BF-42A9-98C6-A7C5DB3701E7", PartitionType::GptFactoryProvisionedreadOnlybootloaderdataFuchsiaStandard),
        GuidAndTypeItem("49FD7CB8-DF15-4E73-B9D9-992070127F0F", PartitionType::GptFuchsiaVolumeManagerFuchsiaStandard),
        GuidAndTypeItem("421A8BFC-85D9-4D85-ACDA-B64EEC0133E9", PartitionType::GptVerifiedbootmetadataFuchsiaStandard),
        GuidAndTypeItem("9B37FFF6-2E58-466A-983A-F7926D0B04E0", PartitionType::GptZirconbootimageFuchsiaStandard),
        GuidAndTypeItem("C12A7328-F81F-11D2-BA4B-00A0C93EC93B", PartitionType::GptFuchsiaEspFuchsiaLegacy),
        GuidAndTypeItem("606B000B-B7C7-4653-A7D5-B737332C899D", PartitionType::GptFuchsiaSystemFuchsiaLegacy),
        GuidAndTypeItem("08185F0C-892D-428A-A789-DBEEC8F55E6A", PartitionType::GptFuchsiaDataFuchsiaLegacy),
        GuidAndTypeItem("48435546-4953-2041-494E-5354414C4C52", PartitionType::GptFuchsiaInstallFuchsiaLegacy),
        GuidAndTypeItem("2967380E-134C-4CBB-B6DA-17E7CE1CA45D", PartitionType::GptFuchsiaBlobFuchsiaLegacy),
        GuidAndTypeItem("41D0E340-57E3-954E-8C1E-17ECAC44CFF5", PartitionType::GptFuchsiaFvmFuchsiaLegacy),
        GuidAndTypeItem("DE30CC86-1F4A-4A31-93C4-66F147D33E05", PartitionType::GptZirconbootimageSlotAFuchsiaLegacy),
        GuidAndTypeItem("23CC04DF-C278-4CE7-8471-897D1A4BCDF7", PartitionType::GptZirconbootimageSlotBFuchsiaLegacy),
        GuidAndTypeItem("A0E5CF57-2DEF-46BE-A80C-A2067C37CD49", PartitionType::GptZirconbootimageSlotRFuchsiaLegacy),
        GuidAndTypeItem("4E5E989E-4C86-11E8-A15B-480FCF35F8E6", PartitionType::GptSysConfigFuchsiaLegacy),
        GuidAndTypeItem("5A3A90BE-4C86-11E8-A15B-480FCF35F8E6", PartitionType::GptFactoryConfigFuchsiaLegacy),
        GuidAndTypeItem("5ECE94FE-4C86-11E8-A15B-480FCF35F8E6", PartitionType::GptBootloaderFuchsiaLegacy),
        GuidAndTypeItem("8B94D043-30BE-4871-9DFA-D69556E8C1F3", PartitionType::GptGuidTestFuchsiaLegacy),
        GuidAndTypeItem("A13B4D9A-EC5F-11E8-97D8-6C3BE52705BF", PartitionType::GptVerifiedbootmetadataSlotAFuchsiaLegacy),
        GuidAndTypeItem("A288ABF2-EC5F-11E8-97D8-6C3BE52705BF", PartitionType::GptVerifiedbootmetadataSlotBFuchsiaLegacy),
        GuidAndTypeItem("6A2460C3-CD11-4E8B-80A8-12CCE268ED0A", PartitionType::GptVerifiedbootmetadataSlotRFuchsiaLegacy),
        GuidAndTypeItem("1D75395D-F2C6-476B-A8B7-45CC1C97B476", PartitionType::GptMiscFuchsiaLegacy),
        GuidAndTypeItem("900B0FC5-90CD-4D4F-84F9-9F8ED579DB88", PartitionType::GptEmmcBoot1FuchsiaLegacy),
        GuidAndTypeItem("B2B2E8D1-7C10-4EBC-A2D0-4614568260AD", PartitionType::GptEmmcBoot2FuchsiaLegacy),
    };

    if (datas.contains(guid))
        return datas.value(guid);
    return PartitionType::PartitionTypeNotFound;
}

QString Utils::getNameByProperty(Property property) {
    const static QMap<Property, QString> datas = {
        PropertyStringItem("Configuration",           Property::BlockConfiguration),
        PropertyStringItem("CryptoBackingDevice",     Property::BlockCryptoBackingDevice),
        PropertyStringItem("Device",                  Property::BlockDevice),
        PropertyStringItem("Drive",                   Property::BlockDrive),
        PropertyStringItem("IdLabel",                 Property::BlockIDLabel),
        PropertyStringItem("IdType",                  Property::BlockIDType),
        PropertyStringItem("IdUsage",                 Property::BlockIDUsage),
        PropertyStringItem("IdUUID",                  Property::BlockIDUUID),
        PropertyStringItem("IdVersion",               Property::BlockIDVersion),
        PropertyStringItem("DeviceNumber",            Property::BlockDeviceNumber),
        PropertyStringItem("PreferredDevice",         Property::BlockPreferredDevice),
        PropertyStringItem("Id",                      Property::BlockID),
        PropertyStringItem("Size",                    Property::BlockSize),
        PropertyStringItem("ReadOnly",                Property::BlockReadOnly),
        PropertyStringItem("Symlinks",                Property::BlockSymlinks),
        PropertyStringItem("HintPartitionable",       Property::BlockHintPartitionable),
        PropertyStringItem("HintSystem",              Property::BlockHintSystem),
        PropertyStringItem("HintIgnore",              Property::BlockHintIgnore),
        PropertyStringItem("HintAuto",                Property::BlockHintAuto),
        PropertyStringItem("HintName",                Property::BlockHintName),
        PropertyStringItem("HintIconName",            Property::BlockHintIconName),
        PropertyStringItem("HintSymbolicIconName",    Property::BlockHintSymbolicIconName),
        PropertyStringItem("MdRaid",                  Property::BlockMdRaid),
        PropertyStringItem("MdRaidMember",            Property::BlockMdRaidMember),
        PropertyStringItem("ConnectionBus",           Property::DriveConnectionBus),
        PropertyStringItem("Removable",               Property::DriveRemovable),
        PropertyStringItem("Ejectable",               Property::DriveEjectable),
        PropertyStringItem("Seat",                    Property::DriveSeat),
        PropertyStringItem("Media",                   Property::DriveMedia),
        PropertyStringItem("MediaCompatibility",      Property::DriveMediaCompatibility),
        PropertyStringItem("MediaRemovable",          Property::DriveMediaRemovable),
        PropertyStringItem("MediaAvailable",          Property::DriveMediaAvailable),
        PropertyStringItem("MediaChangeDetected",     Property::DriveMediaChangeDetected),
        PropertyStringItem("TimeDetected",            Property::DriveTimeDetected),
        PropertyStringItem("TimeMediaDetected",       Property::DriveTimeMediaDetected),
        PropertyStringItem("Size",                    Property::DriveSize),
        PropertyStringItem("Optical",                 Property::DriveOptical),
        PropertyStringItem("OpticalBlank",            Property::DriveOpticalBlank),
        PropertyStringItem("OpticalNumTracks",        Property::DriveOpticalNumTracks),
        PropertyStringItem("OpticalNumAudioTracks",   Property::DriveOpticalNumAudioTracks),
        PropertyStringItem("OpticalNumDataTracks",    Property::DriveOpticalNumDataTracks),
        PropertyStringItem("OpticalNumSessions",      Property::DriveOpticalNumSessions),
        PropertyStringItem("Model",                   Property::DriveModel),
        PropertyStringItem("Revision",                Property::DriveRevision),
        PropertyStringItem("RotationRate",            Property::DriveRotationRate),
        PropertyStringItem("Serial",                  Property::DriveSerial),
        PropertyStringItem("Vender",                  Property::DriveVender),
        PropertyStringItem("WWN",                     Property::DriveWWN),
        PropertyStringItem("SortKey",                 Property::DriveSortKey),
        PropertyStringItem("Configuration",           Property::DriveConfiguration),
        PropertyStringItem("ID",                      Property::DriveID),
        PropertyStringItem("CanPowerOff",             Property::DriveCanPowerOff),
        PropertyStringItem("SiblingID",               Property::DriveSiblingID),
        PropertyStringItem("MountPoints",             Property::FileSystemMountPoint),
        PropertyStringItem("Number",                  Property::PartitionNumber),
        PropertyStringItem("Type",                    Property::PartitionType),
        PropertyStringItem("Offset",                  Property::PartitionOffset),
        PropertyStringItem("Size",                    Property::PartitionSize),
        PropertyStringItem("Flags",                   Property::PartitionFlags),
        PropertyStringItem("Name",                    Property::PartitionName),
        PropertyStringItem("UUID",                    Property::PartitionUUID),
        PropertyStringItem("Table",                   Property::PartitionTable),
        PropertyStringItem("IsContainer",             Property::PartitionIsContainer),
        PropertyStringItem("IsContained",             Property::PartitionIsContained),
        PropertyStringItem("ChildConfiguration",      Property::EncryptedChildConfiguration),
        PropertyStringItem("CleartextDevice",         Property::EncryptedCleartextDevice),
        PropertyStringItem("HintEncryptionType",      Property::EncryptedHintEncryptionType),
        PropertyStringItem("MetadataSize",            Property::EncryptedMetadataSize),
    };
    return datas.value(property, "");
}

QString Utils::errorMessage(DeviceError err)
{
    static const QMap<DeviceError, QString> errors {
        std::pair<DeviceError, QString>(DeviceError::NoError,                    "No error"),
        std::pair<DeviceError, QString>(DeviceError::Failed,                     "Failed"),
        std::pair<DeviceError, QString>(DeviceError::Cancelled,                  "Cancelled"),
        std::pair<DeviceError, QString>(DeviceError::AlreadyCancelled,           "Already cancelled"),
        std::pair<DeviceError, QString>(DeviceError::NotAuthorized,              "Not authorized"),
        std::pair<DeviceError, QString>(DeviceError::NotAuthorizedCanObtain,     "Not authorized can obtain"),
        std::pair<DeviceError, QString>(DeviceError::NotAuthorizedDismissed,     "Not authorized dismissed"),
        std::pair<DeviceError, QString>(DeviceError::AlreadyMounted,             "Device is already mounted"),
        std::pair<DeviceError, QString>(DeviceError::NotMounted,                 "Device is not mounted"),
        std::pair<DeviceError, QString>(DeviceError::OptionNotPermitted,         "Option is not permitted"),
        std::pair<DeviceError, QString>(DeviceError::MountedByOtherUser,         "Device is mounted by other user"),
        std::pair<DeviceError, QString>(DeviceError::AlreadyUnmounting,          "Device is already unmounted"),
        std::pair<DeviceError, QString>(DeviceError::NotSupproted,               "Not supported operation"),
        std::pair<DeviceError, QString>(DeviceError::TimedOut,                   "Operation time out"),
        std::pair<DeviceError, QString>(DeviceError::WouldWakeup,                "Operation would wakeup"),
        std::pair<DeviceError, QString>(DeviceError::DeviceBusy,                 "Device is busy"),
        std::pair<DeviceError, QString>(DeviceError::ScsiDaemonTransportFailed,  "SCSI daemon transport failed"),
        std::pair<DeviceError, QString>(DeviceError::ScsiHostNotFound,           "SCSI host not found"),
        std::pair<DeviceError, QString>(DeviceError::ScsiIDMB,                   "SCSI IDMB"),
        std::pair<DeviceError, QString>(DeviceError::ScsiLoginFailed,            "SCSI login failed"),
        std::pair<DeviceError, QString>(DeviceError::ScsiLoginAuthFailed,        "SCSI login auth failed"),
        std::pair<DeviceError, QString>(DeviceError::ScsiLoginFatal,             "SCSI login fatal"),
        std::pair<DeviceError, QString>(DeviceError::ScsiLogoutFailed,           "SCSI logout failed"),
        std::pair<DeviceError, QString>(DeviceError::ScsiNoFirmware,             "SCSI no firmware found"),
        std::pair<DeviceError, QString>(DeviceError::ScsiNoObjectsFound,         "SCSI no objects found"),
        std::pair<DeviceError, QString>(DeviceError::ScsiNotConnected,           "SCSI not connected"),
        std::pair<DeviceError, QString>(DeviceError::ScsiTransportFailed,        "SCSI transport failed"),
        std::pair<DeviceError, QString>(DeviceError::ScsiUnknownDiscoveryType,   "SCSI unknown discovery type"),
        std::pair<DeviceError, QString>(DeviceError::NotMountable,               "Device is not mountable"),
        std::pair<DeviceError, QString>(DeviceError::NotEjectable,               "Device is not ejectable"),
        std::pair<DeviceError, QString>(DeviceError::NoDriver,                   "Device do not have a drive"),
        std::pair<DeviceError, QString>(DeviceError::NotEncryptable,             "Device is not encryptable"),
        std::pair<DeviceError, QString>(DeviceError::NoPartition,                "Device do not have a partition"),
        std::pair<DeviceError, QString>(DeviceError::NoBlock,                    "Device do not have a block"),
    };
    return errors.value(err);
}

QString Utils::errorMessage(MonitorError err)
{
    static const QMap<MonitorError, QString> errors {
        std::pair<MonitorError, QString>(MonitorError::NoError, "No error"),
        std::pair<MonitorError, QString>(MonitorError::MonitorNotRegister, "Not registered monitor"),
        std::pair<MonitorError, QString>(MonitorError::MonitorAlreadyRegistered, "Monitor is already registered"),
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

