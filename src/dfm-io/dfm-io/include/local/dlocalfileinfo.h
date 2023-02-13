// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    bool initQuerier() DFM_OVERRIDE;
    void initQuerierAsync(int ioPriority = 0, InitQuerierAsyncCallback func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    QVariant attribute(DFileInfo::AttributeID id, bool *success = nullptr) const DFM_OVERRIDE;
    void attributeAsync(DFileInfo::AttributeID id, bool *success = nullptr, int ioPriority = 0, AttributeAsyncCallback func = nullptr, void *userData = nullptr) const DFM_OVERRIDE;

    [[nodiscard]] DFileFuture *initQuerierAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *attributeAsync(AttributeID id, int ioPriority, QObject *parent = nullptr) const DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *attributeAsync(const QByteArray &key, const DFileAttributeType type, int ioPriority, QObject *parent = nullptr) const DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *existsAsync(int ioPriority, QObject *parent = nullptr) const DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *refreshAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;
    [[nodiscard]] DFileFuture *permissionsAsync(int ioPriority, QObject *parent = nullptr) DFM_OVERRIDE;

    bool setAttribute(DFileInfo::AttributeID id, const QVariant &value) DFM_OVERRIDE;
    bool hasAttribute(DFileInfo::AttributeID id) const DFM_OVERRIDE;
    bool exists() const DFM_OVERRIDE;
    bool refresh() DFM_OVERRIDE;
    DFile::Permissions permissions() const DFM_OVERRIDE;
    // custom attribute
    bool setCustomAttribute(const char *key, const DFileAttributeType type, const void *value, const FileQueryInfoFlags flag = FileQueryInfoFlags::kTypeNone) DFM_OVERRIDE;
    QVariant customAttribute(const char *key, const DFileAttributeType type) const DFM_OVERRIDE;
    DFMIOError lastError() const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalFileInfoPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALFILEINFO_H
