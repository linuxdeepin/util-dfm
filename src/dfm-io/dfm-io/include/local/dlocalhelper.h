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

template<class C, typename Ret, typename... Ts>
std::function<Ret(Ts...)> bind_field(C *c, Ret (C::*m)(Ts...))
{
    return [=](auto &&... args) { return (c->*m)(std::forward<decltype(args)>(args)...); };
}

template<class C, typename Ret, typename... Ts>
std::function<Ret(Ts...)> bind_field(const C *c, Ret (C::*m)(Ts...) const)
{
    return [=](auto &&... args) { return (c->*m)(std::forward<decltype(args)>(args)...); };
}

template<typename Ret, typename... Ts>
std::function<Ret(Ts...)> bind_field(Ret (*m)(Ts...))
{
    return [=](auto &&... args) { return (*m)(std::forward<decltype(args)>(args)...); };
}

class DLocalHelper
{
public:
    static QSharedPointer<DFileInfo> createFileInfoByUri(const QUrl &uri, const char *attributes = "*",
                                                         const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone);
    static QSharedPointer<DFileInfo> createFileInfoByUri(const QUrl &uri, GFileInfo *gfileInfo, const char *attributes = "*",
                                                         const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone);

    static QVariant attributeFromGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, DFMIOErrorCode &errorcode);
    static QVariant customAttributeFromPath(const QString &path, DFileInfo::AttributeID id);
    static QVariant customAttributeFromPathAndInfo(const QString &path, GFileInfo *fileInfo, DFileInfo::AttributeID id);
    static bool setAttributeByGFile(GFile *gfile, DFileInfo::AttributeID id, const QVariant &value, GError **error);
    static bool setAttributeByGFileInfo(GFileInfo *gfileinfo, DFileInfo::AttributeID id, const QVariant &value);
    static std::string attributeStringById(DFileInfo::AttributeID id);
    static QSet<QString> hideListFromUrl(const QUrl &url);
    static bool fileIsHidden(const QSharedPointer<DFileInfo> &dfileinfo, const QSet<QString> &hideList);

    // tools
    static bool checkGFileType(GFile *file, GFileType type);
};

END_IO_NAMESPACE

#endif   // DGIOHELPER_H
