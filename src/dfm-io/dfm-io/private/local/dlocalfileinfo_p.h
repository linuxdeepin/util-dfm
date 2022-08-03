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

#ifndef DLOCALFILEINFO_P_H
#define DLOCALFILEINFO_P_H

#include "dfmio_global.h"

#include "gio/gio.h"

#include "local/dlocalfileinfo.h"
#include "core/dfileinfo.h"

#include <QMap>

BEGIN_IO_NAMESPACE

class DLocalFileInfo;

class DLocalFileInfoPrivate : public QObject
{
public:
    explicit DLocalFileInfoPrivate(DLocalFileInfo *q);
    ~DLocalFileInfoPrivate();

    void initNormal();
    bool queryInfoSync();

    void queryInfoAsync(int ioPriority = 0, DFileInfo::QueryInfoAsyncCallback func = nullptr, void *userData = nullptr);
    QVariant attribute(DFileInfo::AttributeID id, bool *success = nullptr);
    bool setAttribute(DFileInfo::AttributeID id, const QVariant &value);
    bool hasAttribute(DFileInfo::AttributeID id);
    bool removeAttribute(DFileInfo::AttributeID id);
    QList<DFileInfo::AttributeID> attributeIDList() const;
    bool exists() const;
    bool refresh();
    bool clearCache();
    DFile::Permissions permissions();

    bool setCustomAttribute(const char *key, const DFileInfo::DFileAttributeType type, const void *value, const DFileInfo::FileQueryInfoFlags flag = DFileInfo::FileQueryInfoFlags::kTypeNone);
    QVariant customAttribute(const char *key, const DFileInfo::DFileAttributeType type);

    DFMIOError lastError();
    void setErrorFromGError(GError *gerror);

    void freeCancellable(GCancellable *gcancellable);

private:
    bool cacheAttribute(DFileInfo::AttributeID id, const QVariant &value);
    QVariant attributesBySelf(DFileInfo::AttributeID id);

public:
    QMap<DFileInfo::AttributeID, QVariant> attributesCache;
    QList<DFileInfo::AttributeID> attributesRealizationSelf;
    GFile *gfile = nullptr;
    GFileInfo *gfileinfo = nullptr;
    bool initFinished = false;
    bool infoReseted = false;
    GCancellable *gcancellable = nullptr;

    DFMIOError error;

    DLocalFileInfo *q = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALFILEINFO_P_H
