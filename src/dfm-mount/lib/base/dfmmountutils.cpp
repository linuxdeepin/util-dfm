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
