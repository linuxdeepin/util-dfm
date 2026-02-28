// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDEVICE_H
#define DDEVICE_H

#include <dfm-mount/base/dmount_global.h>

#include <QObject>
#include <QSharedPointer>
#include <QUrl>
#include <QMap>
#include <QVariant>

#include <functional>

DFM_MOUNT_BEGIN_NS

class DDevicePrivate;
class DDevice : public QObject
{
    Q_OBJECT

public:
    explicit DDevice(DDevicePrivate *dd, QObject *parent = nullptr);
    virtual ~DDevice();

    DMNT_VIRTUAL QString path() const;
    DMNT_VIRTUAL QString mount(const QVariantMap &opts = {});
    DMNT_VIRTUAL void mountAsync(const QVariantMap &opts = {}, DeviceOperateCallbackWithMessage cb = nullptr);
    DMNT_VIRTUAL bool unmount(const QVariantMap &opts = {});
    DMNT_VIRTUAL void unmountAsync(const QVariantMap &opts = {}, DeviceOperateCallback cb = nullptr);
    DMNT_VIRTUAL bool rename(const QString &newName, const QVariantMap &opts = {});
    DMNT_VIRTUAL void renameAsync(const QString &newName, const QVariantMap &opts = {}, DeviceOperateCallback cb = nullptr);
    DMNT_VIRTUAL QString mountPoint() const;
    DMNT_VIRTUAL QString fileSystem() const;
    DMNT_VIRTUAL qint64 sizeTotal() const;

    // use these two functions CAUTION!!! for block devices if you invoke just after mount sync, the
    // mount point of this device cannot be returned correctly, so there is no way to get the useage
    // via QStorageInfo cause cannot get the mountpoint right away.
    // already submit an issue to report this: https://github.com/storaged-project/udisks/issues/930
    DMNT_VIRTUAL qint64 sizeFree() const;
    DMNT_VIRTUAL qint64 sizeUsage() const;

    DMNT_VIRTUAL DeviceType deviceType() const;
    DMNT_VIRTUAL QVariant getProperty(Property name) const;
    DMNT_VIRTUAL QString displayName() const;

    OperationErrorInfo lastError() const;

public:
    // type definition
    using PathFunc = std::function<QString()>;
    using MountFunc = std::function<QString(const QVariantMap &)>;
    using MountAsyncFunc = std::function<void(const QVariantMap &, DeviceOperateCallbackWithMessage)>;
    using UnmountFunc = std::function<bool(const QVariantMap &)>;
    using UnmountAsyncFunc = std::function<void(const QVariantMap &, DeviceOperateCallback)>;
    using RenameFunc = std::function<bool(const QString &, const QVariantMap &)>;
    using RenameAsyncFunc = std::function<void(const QString &, const QVariantMap &, DeviceOperateCallback)>;
    using MountPointFunc = std::function<QString()>;
    using FileSystemFunc = std::function<QString()>;
    using SizeTotalFunc = std::function<qint64()>;
    using SizeUsageFunc = std::function<qint64()>;
    using SizeFreeFunc = std::function<qint64()>;
    using DeviceTypeFunc = std::function<DeviceType()>;
    using GetPropertyFunc = std::function<QVariant(Property)>;
    using DisplayNameFunc = std::function<QString()>;

    void registerPath(const PathFunc &func);
    void registerMount(const MountFunc &func);
    void registerMountAsync(const MountAsyncFunc &func);
    void registerUnmount(const UnmountFunc &func);
    void registerUnmountAsync(const UnmountAsyncFunc &func);
    void registerRename(const RenameFunc &func);
    void registerRenameAsync(const RenameAsyncFunc &func);
    void registerMountPoint(const MountPointFunc &func);
    void registerFileSystem(const FileSystemFunc &func);
    void registerSizeTotal(const SizeTotalFunc &func);
    void registerSizeUsage(const SizeUsageFunc &func);
    void registerSizeFree(const SizeFreeFunc &func);
    void registerDeviceType(const DeviceTypeFunc &func);
    void registerGetProperty(const GetPropertyFunc &func);
    void registerDisplayName(const DisplayNameFunc &func);

protected:
    QScopedPointer<DDevicePrivate> d;
};
DFM_MOUNT_END_NS

#endif   // DDEVICE_H
