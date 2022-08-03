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

#ifndef DLOCALFILEINFO_H
#define DLOCALFILEINFO_H

#include "dfmio_global.h"

#include "core/dfileinfo.h"

BEGIN_IO_NAMESPACE

class DLocalFileInfoPrivate;

class DLocalFileInfo : public DFileInfo
{
public:
    explicit DLocalFileInfo(const QUrl &uri, const char *attributes = "*",
                            const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone);
    explicit DLocalFileInfo(const QUrl &uri, void *fileInfo,
                            const char *attributes = "*", const DFMIO::DFileInfo::FileQueryInfoFlags flag = DFMIO::DFileInfo::FileQueryInfoFlags::kTypeNone);

    virtual ~DLocalFileInfo();

    void queryInfoAsync(int ioPriority = 0, QueryInfoAsyncCallback func = nullptr, void *userData = nullptr) const DFM_OVERRIDE;
    QVariant attribute(DFileInfo::AttributeID id, bool *success = nullptr) DFM_OVERRIDE;
    bool setAttribute(DFileInfo::AttributeID id, const QVariant &value) DFM_OVERRIDE;
    bool hasAttribute(DFileInfo::AttributeID id) DFM_OVERRIDE;
    bool removeAttribute(DFileInfo::AttributeID id) DFM_OVERRIDE;
    QList<DFileInfo::AttributeID> attributeIDList() const DFM_OVERRIDE;
    bool exists() const DFM_OVERRIDE;
    bool refresh() DFM_OVERRIDE;
    bool clearCache() DFM_OVERRIDE;
    DFile::Permissions permissions() DFM_OVERRIDE;
    // custom attribute
    bool setCustomAttribute(const char *key, const DFileAttributeType type, const void *value, const FileQueryInfoFlags flag = FileQueryInfoFlags::kTypeNone) DFM_OVERRIDE;
    QVariant customAttribute(const char *key, const DFileAttributeType type) DFM_OVERRIDE;
    DFMIOError lastError() const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalFileInfoPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALFILEINFO_H
