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

#include "local/dlocalfileinfo.h"
#include "local/dlocalfileinfo_p.h"
#include "local/dlocalhelper.h"

#include <QDebug>

USING_IO_NAMESPACE

DLocalFileInfoPrivate::DLocalFileInfoPrivate(DLocalFileInfo *q)
    : q(q)
{

}

DLocalFileInfoPrivate::~DLocalFileInfoPrivate()
{
}

bool DLocalFileInfoPrivate::init()
{
    const QUrl &url = q->uri();
    const QString &path = url.toString();

    GFile *file = g_file_new_for_uri(path.toLocal8Bit().data());

    GError *gerror = nullptr;

    GFileInfo *gfileinfo = g_file_query_info(file, "*", G_FILE_QUERY_INFO_NONE, nullptr, &gerror);
    g_object_unref(file);

    if (gerror)
        g_error_free(gerror);

    if (!gfileinfo)
        return false;

    this->gfileinfo = gfileinfo;
    return true;
}

QVariant DLocalFileInfoPrivate::attribute(DFileInfo::AttributeID id, bool *success)
{
    *success = false;

    if (attributes.count(id) == 0) {
        if (gfileinfo) {
            QVariant value;
            if (id > DFileInfo::AttributeID::CustomStart) {
                const QString &path = q->uri().path();
                value = DLocalHelper::customAttributeFromPath(path, id);
            } else {
                value = DLocalHelper::attributeFromGFileInfo(gfileinfo, id);
            }
            setAttribute(id, value);
            *success = true;
            return value;
        }
        return QVariant();
    }
    *success = true;
    return attributes.value(id);
}

bool DLocalFileInfoPrivate::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    if (attributes.count(id) > 0)
        attributes.remove(id);

    attributes.insert(id, value);
    return true;
}

bool DLocalFileInfoPrivate::hasAttribute(DFileInfo::AttributeID id)
{
    if (attributes.count(id) > 0)
        return true;

    if (gfileinfo) {
        const QVariant &value = DLocalHelper::attributeFromGFileInfo(gfileinfo, id);
        setAttribute(id, value);
        return true;
    }
    return false;
}

bool DLocalFileInfoPrivate::removeAttribute(DFileInfo::AttributeID id)
{
    if (attributes.count(id) > 0)
        attributes.remove(id);
    return true;
}

QList<DFileInfo::AttributeID> DLocalFileInfoPrivate::attributeIDList() const
{
    return attributes.keys();
}

bool DLocalFileInfoPrivate::exists() const
{
    const QUrl &url = q->uri();
    const QString &path = url.toString();

    GFile *gfile = g_file_new_for_path(path.toLocal8Bit().data());
    const bool exists = g_file_query_exists(gfile, nullptr);

    g_object_unref(gfile);
    return exists;
}

bool DLocalFileInfoPrivate::flush()
{
    auto it = attributes.constBegin();
    while (it != attributes.constEnd()) {
        DLocalHelper::setAttributeByGFileInfo(gfileinfo, it.key(), it.value());
        ++it;
    }
    return true;
}
DLocalFileInfo::DLocalFileInfo(const QUrl &uri) : DFileInfo(uri)
    , d(new DLocalFileInfoPrivate(this))
{
    registerAttribute(std::bind(&DLocalFileInfo::attribute, this, std::placeholders::_1, std::placeholders::_2));
    registerSetAttribute(std::bind(&DLocalFileInfo::setAttribute, this, std::placeholders::_1, std::placeholders::_2));
    registerHasAttribute(std::bind(&DLocalFileInfo::hasAttribute, this, std::placeholders::_1));
    registerRemoveAttribute(std::bind(&DLocalFileInfo::removeAttribute, this, std::placeholders::_1));
    registerAttributeList(std::bind(&DLocalFileInfo::attributeIDList, this));
    registerExists(std::bind(&DLocalFileInfo::exists, this));
    registerFlush(std::bind(&DLocalFileInfo::flush, this));

    d->init();
}

DLocalFileInfo::~DLocalFileInfo()
{

}

QVariant DLocalFileInfo::attribute(DFileInfo::AttributeID id, bool *success /*= nullptr */)
{
    return d->attribute(id, success);
}

bool DLocalFileInfo::setAttribute(DFileInfo::AttributeID id, const QVariant &value)
{
    return d->setAttribute(id, value);
}

bool DLocalFileInfo::hasAttribute(DFileInfo::AttributeID id)
{
    return d->hasAttribute(id);
}

bool DLocalFileInfo::removeAttribute(DFileInfo::AttributeID id)
{
    return d->removeAttribute(id);
}

QList<DFileInfo::AttributeID> DLocalFileInfo::attributeIDList() const
{
    return d->attributeIDList();
}

bool DLocalFileInfo::exists() const
{
    return d->exists();
}

bool DLocalFileInfo::flush()
{
    return d->flush();
}
