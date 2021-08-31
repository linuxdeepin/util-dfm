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
using namespace std;
class DFMDevicePrivate;
class DFMDevice: public QObject
{
    Q_OBJECT
public:
    explicit DFMDevice(QObject *parent = nullptr);
    virtual ~DFMDevice(){}

    DFM_MNT_VIRTUAL QString path();

    DFM_MNT_VIRTUAL QUrl mount(const QVariantMap &opts);
    DFM_MNT_VIRTUAL void mountAsync(const QVariantMap &opts);

    DFM_MNT_VIRTUAL bool unmount();
    DFM_MNT_VIRTUAL void unmountAsync();

    DFM_MNT_VIRTUAL bool rename(const QString &newName);
    DFM_MNT_VIRTUAL void renameAsync(const QString &newName);

    DFM_MNT_VIRTUAL QUrl accessPoint();
    DFM_MNT_VIRTUAL QUrl mountPoint();
    DFM_MNT_VIRTUAL QString fileSystem();
    DFM_MNT_VIRTUAL long sizeTotal();
    DFM_MNT_VIRTUAL long sizeFree();
    DFM_MNT_VIRTUAL long sizeUsage();

    DFM_MNT_VIRTUAL int deviceType();
    DFM_MNT_VIRTUAL QVariant getProperty(Property name);

public:

    // type definition
    using Path = function<QString ()>;
    using Mount = function<QUrl (const QVariantMap &)>;
    using MountAsync = function<void (const QVariantMap &)>;
    using Unmount = function<bool ()>;
    using UnmountAsync = function<void ()>;
    using Rename = function<bool (const QString &)>;
    using RenameAsync = function<void (const QString &)>;
    using AccessPoint = function<QUrl ()>;
    using MountPoint = function<QUrl ()>;
    using FileSystem = function<QString ()>;
    using SizeTotal = function<long ()>;
    using SizeUsage = function<long ()>;
    using SizeFree = function<long ()>;
    using DeviceType = function<int ()>;
    using GetProperty = function<QVariant (Property)>;

    void registerPath(const Path &func);
    void registerMount(const Mount &func);
    void registerMountAsync(const MountAsync &func);
    void registerUnmount(const Unmount &func);
    void registerUnmountAsync(const UnmountAsync &func);
    void registerRename(const Rename &func);
    void registerRenameAsync(const RenameAsync &func);
    void registerAccessPoint(const AccessPoint &func);
    void registerMountPoint(const MountPoint &func);
    void registerFileSystem(const FileSystem &func);
    void registerSizeTotal(const SizeTotal &func);
    void registerSizeUsage(const SizeUsage &func);
    void registerSizeFree(const SizeFree &func);
    void registerDeviceType(const DeviceType &func);
    void registerGetProperty(const GetProperty &func);


Q_SIGNALS:
    void mounted(const QUrl &mountPoint);
    void unmounted();
    void renamed(const QString &newName);

protected:
    QSharedPointer<DFMDevicePrivate> d_ptr = nullptr;
    Q_DECLARE_PRIVATE(DFMDevice)
};
DFM_MOUNT_END_NS

#endif // DFMDEVICE_H
