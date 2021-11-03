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
    };
    return datas.value(name, Property::NotInit);
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
