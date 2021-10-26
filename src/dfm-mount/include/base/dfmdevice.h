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
#ifndef DFMDEVICE_H
#define DFMDEVICE_H

#include "dfmmount_global.h"
#include "dfmmountdefines.h"

#include <QObject>
#include <QSharedPointer>
#include <QUrl>
#include <QMap>
#include <QVariant>

#include <functional>

DFM_MOUNT_BEGIN_NS

class DFMDevicePrivate;
class DFMDevice: public QObject
{
    Q_OBJECT

public:
    explicit DFMDevice(DFMDevicePrivate *dd, QObject *parent = nullptr);
    virtual ~DFMDevice();

    DFM_MNT_VIRTUAL QString path() const;

    DFM_MNT_VIRTUAL QString mount(const QVariantMap &opts = {});
    DFM_MNT_VIRTUAL void mountAsync(const QVariantMap &opts = {}, DeviceOperateCb cb = nullptr);

    DFM_MNT_VIRTUAL bool unmount(const QVariantMap &opts = {});
    DFM_MNT_VIRTUAL void unmountAsync(const QVariantMap &opts = {}, DeviceOperateCb cb = nullptr);

    DFM_MNT_VIRTUAL bool rename(const QString &newName, const QVariantMap &opts = {});
    DFM_MNT_VIRTUAL void renameAsync(const QString &newName, const QVariantMap &opts = {}, DeviceOperateCb cb = nullptr);

    DFM_MNT_VIRTUAL QString mountPoint() const;
    DFM_MNT_VIRTUAL QString fileSystem() const;
    DFM_MNT_VIRTUAL qint64 sizeTotal() const;

    // use these two functions CAUTION!!! for block devices if you invoke just after mount sync, the
    // mount point of this device cannot be returned correctly, so there is no way to get the useage
    // via QStorageInfo cause cannot get the mountpoint right away.
    // already submit an issue to report this: https://github.com/storaged-project/udisks/issues/930
    DFM_MNT_VIRTUAL qint64 sizeFree() const;
    DFM_MNT_VIRTUAL qint64 sizeUsage() const;

    DFM_MNT_VIRTUAL DeviceType deviceType() const;
    DFM_MNT_VIRTUAL QVariant getProperty(Property name) const;
    DeviceError getLastError() const;

public:

    // type definition
    using PathFunc         = std::function<QString ()>;
    using MountFunc        = std::function<QString (const QVariantMap &)>;
    using MountAsyncFunc   = std::function<void (const QVariantMap &, DeviceOperateCb)>;
    using UnmountFunc      = std::function<bool (const QVariantMap &)>;
    using UnmountAsyncFunc = std::function<void (const QVariantMap &, DeviceOperateCb)>;
    using RenameFunc       = std::function<bool (const QString &, const QVariantMap &)>;
    using RenameAsyncFunc  = std::function<void (const QString &, const QVariantMap &, DeviceOperateCb)>;
    using MountPointFunc   = std::function<QString ()>;
    using FileSystemFunc   = std::function<QString ()>;
    using SizeTotalFunc    = std::function<qint64 ()>;
    using SizeUsageFunc    = std::function<qint64 ()>;
    using SizeFreeFunc     = std::function<qint64 ()>;
    using DeviceTypeFunc   = std::function<DeviceType ()>;
    using GetPropertyFunc  = std::function<QVariant (Property)>;

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

protected:
    QScopedPointer<DFMDevicePrivate> d;
};
DFM_MOUNT_END_NS

#endif // DFMDEVICE_H
