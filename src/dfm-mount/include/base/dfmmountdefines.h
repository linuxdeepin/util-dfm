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
#ifndef DFMMOUNTDEFINES_H
#define DFMMOUNTDEFINES_H

#include "dfmmount_global.h"

#include <QtCore/QtGlobal>
#include <QVariant>
#include <QDebug>
#include <glib.h>

DFM_MOUNT_BEGIN_NS

template<typename ParentPrivate, typename SubPrivate>
inline SubPrivate *castSubPrivate(ParentPrivate *p) {
    auto pPointer = dynamic_cast<SubPrivate *>(p);
    if (!pPointer)
        abort();
    return pPointer;
}

inline QVariant gvariantToQVariant(GVariant *val) {
    if (!val)
        return QVariant();
    GVariantClass type = g_variant_classify(val);
    switch (type) {
    case G_VARIANT_CLASS_BOOLEAN:
        return QVariant(static_cast<bool>(g_variant_get_boolean(val)));
    case G_VARIANT_CLASS_BYTE:
        return QVariant(static_cast<char>(g_variant_get_byte(val)));
    case G_VARIANT_CLASS_INT16:
        return QVariant(static_cast<int>(g_variant_get_int16(val)));
    case G_VARIANT_CLASS_UINT16:
        return QVariant(static_cast<uint>(g_variant_get_uint16(val)));
    case G_VARIANT_CLASS_INT32:
        return QVariant(static_cast<int>(g_variant_get_int32(val)));
    case G_VARIANT_CLASS_UINT32:
        return QVariant(static_cast<uint>(g_variant_get_uint32(val)));
    case G_VARIANT_CLASS_INT64:
        return QVariant(static_cast<int>(g_variant_get_int64(val)));
    case G_VARIANT_CLASS_UINT64:
        return QVariant(static_cast<uint>(g_variant_get_uint64(val)));
    case G_VARIANT_CLASS_DOUBLE:
        return QVariant(static_cast<double>(g_variant_get_double(val)));
    case G_VARIANT_CLASS_STRING:
        return QString(g_variant_get_string(val, nullptr));
    case G_VARIANT_CLASS_OBJECT_PATH:
        return QString(g_variant_get_string(val, nullptr));
    case G_VARIANT_CLASS_ARRAY: {
        gsize itemCount = g_variant_n_children(val);
        QVariantList lst;
        for (ulong i = 0; i < itemCount; i++) {
            GVariant *item = g_variant_get_child_value(val, i);
            if (G_VARIANT_CLASS_ARRAY == g_variant_classify(item)) {
                const char *byteString = g_variant_get_bytestring(item);
                if (byteString)
                    lst << byteString;
                else
                    lst << gvariantToQVariant(item);
            }
        }
        return lst;
    }
    case G_VARIANT_CLASS_TUPLE:
    case G_VARIANT_CLASS_DICT_ENTRY:
    case G_VARIANT_CLASS_SIGNATURE:
    case G_VARIANT_CLASS_VARIANT:
    case G_VARIANT_CLASS_MAYBE:
    case G_VARIANT_CLASS_HANDLE:
        qDebug() << "no mapped type: " << type;
        return QVariant();
    }
}

DFM_MOUNT_END_NS

#endif // DFMMOUNTDEFINES_H
