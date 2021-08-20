/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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

#include "core/dfileinfo_p.h"

USING_IO_NAMESPACE

DFileInfoPrivate::AttrNameMap DFileInfoPrivate::attrNames = {
    //std::make_pair(DFileInfo::AttributeID::StandardType, "type"),
    {DFileInfo::AttributeID::StandardType, "type"},
    {DFileInfo::AttributeID::StandardIsHiden, "is-hiden"},
    {DFileInfo::AttributeID::StandardName, "name"},
    {DFileInfo::AttributeID::StandardDisplayName, "display-name"},
    {DFileInfo::AttributeID::StandardEditName, "edit-name"},
    {DFileInfo::AttributeID::StandardAllocatedSize, "allocated-size"},
    {DFileInfo::AttributeID::StandardIcon, "icon"},
    {DFileInfo::AttributeID::StandardContentType, "content-type"},
    {DFileInfo::AttributeID::StandardFastContentType, "fast-content-type"},
    {DFileInfo::AttributeID::StandardSize, "size"},
    {DFileInfo::AttributeID::AccessCanTrash, "can-trash"},
    {DFileInfo::AttributeID::AccessCanRead, "can-read"},
    {DFileInfo::AttributeID::AccessCanWrite, "can-write"},
    {DFileInfo::AttributeID::AccessCanExecute, "can-execute"},
    {DFileInfo::AttributeID::AccessCanDelete, "can-delete"},
    {DFileInfo::AttributeID::AccessCanRename, "can-rename"},
    {DFileInfo::AttributeID::TimeModified, "modified"},
    {DFileInfo::AttributeID::TimeAccess, "access"},
    {DFileInfo::AttributeID::TimeChanged, "change"},
    {DFileInfo::AttributeID::OwnerUser, "user"},
    {DFileInfo::AttributeID::OwnerGroup, "group"},
    {DFileInfo::AttributeID::UnixMode, "mode"},
    {DFileInfo::AttributeID::UnixUID, "uid"},
    {DFileInfo::AttributeID::UnixGID, "gid"},
    {DFileInfo::AttributeID::CustomStart, "custom-start"},
};

QString DFileInfoPrivate::attributeName(DFileInfo::AttributeID id) const
{
    if (attrNames.count(id) > 0)
        return QString::fromLocal8Bit(attrNames.at(id).c_str());
    return "";
}

DFileInfo::DFileInfo()
    : d_ptr(new DFileInfoPrivate(this))
{
}

DFileInfo::DFileInfo(const QUrl &uri)
    : d_ptr(new DFileInfoPrivate(this))
{
    d_ptr->uri = uri;
}

DFileInfo::DFileInfo(const DFileInfo &info)
    : d_ptr(info.d_ptr)
{

}

DFileInfo::~DFileInfo()
{

}

DFileInfo &DFileInfo::operator=(const DFileInfo &info)
{
    d_ptr = info.d_ptr;
    return *this;
}

QVariant DFileInfo::attribute(DFileInfo::AttributeID id, bool &success) const
{
    success = false;
    if (d_ptr->attributes.count(id) > 0) {
        success = true;
        return d_ptr->attributes.value(id);
    }

    return QVariant();
}

bool DFileInfo::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    d_ptr->attributes[id] = value;
    return true;
}

bool DFileInfo::hasAttribute(DFileInfo::AttributeID id) const
{
    return d_ptr->attributes.count(id);
}

bool DFileInfo::removeAttribute(DFileInfo::AttributeID id)
{
    return d_ptr->attributes.remove(id);
}

QList<DFileInfo::AttributeID> DFileInfo::attributeIDList() const
{
    return d_ptr->attributes.keys();
}

QUrl DFileInfo::uri() const
{
    return d_ptr->uri;
}

QString DFileInfo::dump() const
{
    QString ret;
    QMap<AttributeID, QVariant>::const_iterator iter = d_ptr->attributes.begin();
    while (iter != d_ptr->attributes.end()) {
        ret.append(attributeName(iter.key()));
        ret.append(":");
        ret.append(iter.value().toString());
        ret.append("\n");
        ++iter;
    }
    return ret;
}

QString DFileInfo::attributeName(DFileInfo::AttributeID id) const
{
    return d_ptr->attributeName(id);
}

DFMIOError DFileInfo::lastError() const
{
    return d_ptr->error;
}
