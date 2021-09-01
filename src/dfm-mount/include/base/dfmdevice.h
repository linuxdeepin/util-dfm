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

#include <QObject>
#include <QSharedPointer>
#include <QUrl>

#include <functional>

DFM_MOUNT_BEGIN_NS

class DFMDevicePrivate;
class DFMDevice: public QObject
{
    Q_OBJECT
public:
    explicit DFMDevice(QObject *parent = nullptr);
    virtual ~DFMDevice() {}

    DFM_MNT_VIRTUAL QString path() const;

    DFM_MNT_VIRTUAL QUrl mount(const QVariantMap &opts);
    DFM_MNT_VIRTUAL void mountAsync(const QVariantMap &opts);

    DFM_MNT_VIRTUAL bool unmount();
    DFM_MNT_VIRTUAL void unmountAsync();

    DFM_MNT_VIRTUAL bool rename(const QString &newName);
    DFM_MNT_VIRTUAL void renameAsync(const QString &newName);

    DFM_MNT_VIRTUAL QUrl accessPoint() const;
    DFM_MNT_VIRTUAL QUrl mountPoint() const;
    DFM_MNT_VIRTUAL QString fileSystem() const;
    DFM_MNT_VIRTUAL long sizeTotal() const;
    DFM_MNT_VIRTUAL long sizeFree() const;
    DFM_MNT_VIRTUAL long sizeUsage() const;

    DFM_MNT_VIRTUAL DeviceType deviceType() const;
    DFM_MNT_VIRTUAL QVariant getProperty(Property name) const;

public:

    // type definition
    using PathFunc         = std::function<QString ()>;
    using MountFunc        = std::function<QUrl (const QVariantMap &)>;
    using MountAsyncFunc   = std::function<void (const QVariantMap &)>;
    using UnmountFunc      = std::function<bool ()>;
    using UnmountAsyncFunc = std::function<void ()>;
    using RenameFunc       = std::function<bool (const QString &)>;
    using RenameAsyncFunc  = std::function<void (const QString &)>;
    using AccessPointFunc  = std::function<QUrl ()>;
    using MountPointFunc   = std::function<QUrl ()>;
    using FileSystemFunc   = std::function<QString ()>;
    using SizeTotalFunc    = std::function<long ()>;
    using SizeUsageFunc    = std::function<long ()>;
    using SizeFreeFunc     = std::function<long ()>;
    using DeviceTypeFunc   = std::function<DeviceType ()>;
    using GetPropertyFunc  = std::function<QVariant (Property)>;

    void registerPath(const PathFunc &func);
    void registerMount(const MountFunc &func);
    void registerMountAsync(const MountAsyncFunc &func);
    void registerUnmount(const UnmountFunc &func);
    void registerUnmountAsync(const UnmountAsyncFunc &func);
    void registerRename(const RenameFunc &func);
    void registerRenameAsync(const RenameAsyncFunc &func);
    void registerAccessPoint(const AccessPointFunc &func);
    void registerMountPoint(const MountPointFunc &func);
    void registerFileSystem(const FileSystemFunc &func);
    void registerSizeTotal(const SizeTotalFunc &func);
    void registerSizeUsage(const SizeUsageFunc &func);
    void registerSizeFree(const SizeFreeFunc &func);
    void registerDeviceType(const DeviceTypeFunc &func);
    void registerGetProperty(const GetPropertyFunc &func);


Q_SIGNALS:
    void mounted(const QUrl &mountPoint);
    void unmounted();
    void renamed(const QString &newName);

private:
    QSharedPointer<DFMDevicePrivate> d_pointer = nullptr;
    Q_DECLARE_PRIVATE(DFMDevice)
};
DFM_MOUNT_END_NS

#endif // DFMDEVICE_H
