/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#ifndef DGIOHELPER_H
#define DGIOHELPER_H

#include "dfmio_global.h"
#include "core/dfileinfo.h"

#include <gio/gio.h>

#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DLocalHelper
{
public:
    static QSharedPointer<DFileInfo> getFileInfo(const QString &uri);
    static QSharedPointer<DFileInfo> getFileInfoByUri(const QString &uri);
    static GFileInfo *getFileInfoFromDFileInfo(const DFileInfo &dfileinfo);

    static QVariant attributeFromGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id);
    static QVariant customAttributeFromPath(const QString &path, DFileInfo::AttributeID id);
    static void setAttributeByGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, const QVariant &value);
    static std::string attributeStringById(DFileInfo::AttributeID id);

    // tools
    static bool checkGFileType(GFile *file, GFileType type);
};

END_IO_NAMESPACE

#endif // DGIOHELPER_H
