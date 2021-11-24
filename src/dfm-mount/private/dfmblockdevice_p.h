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
#ifndef DFMBLOCKDEVICE_P_H
#define DFMBLOCKDEVICE_P_H

#include "dfmblockdevice.h"
#include "udisks/udisks-generated.h"
#include "private/dfmdevice_p.h"
#include "base/dfmmountdefines.h"

DFM_MOUNT_BEGIN_NS

class DFMBlockDevicePrivate final: public DFMDevicePrivate
{
public:
    DFMBlockDevicePrivate(UDisksClient *cli, const QString &blkObjPath, DFMBlockDevice *qq);
    ~DFMBlockDevicePrivate();

    QString path() const DFM_MNT_OVERRIDE;
    QString mount(const QVariantMap &opts) DFM_MNT_OVERRIDE;
    void mountAsync(const QVariantMap &opts, DeviceOperateCb cb) DFM_MNT_OVERRIDE;
    bool unmount(const QVariantMap &opts) DFM_MNT_OVERRIDE;
    void unmountAsync(const QVariantMap &opts, DeviceOperateCb cb) DFM_MNT_OVERRIDE;
    bool rename(const QString &newName, const QVariantMap &opts) DFM_MNT_OVERRIDE;
    void renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCb cb) DFM_MNT_OVERRIDE;
    QString mountPoint() const DFM_MNT_OVERRIDE;
    QString fileSystem() const DFM_MNT_OVERRIDE;
    qint64 sizeTotal() const DFM_MNT_OVERRIDE;
    qint64 sizeUsage() const DFM_MNT_OVERRIDE;
    qint64 sizeFree() const DFM_MNT_OVERRIDE;
    DeviceType deviceType() const DFM_MNT_OVERRIDE;
    QVariant getProperty(Property name) const DFM_MNT_OVERRIDE;
    QString displayName() const DFM_MNT_OVERRIDE;

    QVariant getBlockProperty(Property name) const;
    QVariant getDriveProperty(Property name) const;
    QVariant getFileSystemProperty(Property name) const;
    QVariant getPartitionProperty(Property name) const;
    QVariant getEncryptedProperty(Property name) const;

    bool eject(const QVariantMap &opts);
    void ejectAsync(const QVariantMap &opts, DeviceOperateCb cb);
    bool powerOff(const QVariantMap &opts);
    void powerOffAsync(const QVariantMap &opts, DeviceOperateCb cb);
    bool lock(const QVariantMap &opts);
    void lockAsync(const QVariantMap &opts, DeviceOperateCb cb);
    bool unlock(const QString &passwd, QString &clearTextDev, const QVariantMap &opts);
    void unlockAsync(const QString &passwd, const QVariantMap &opts, DeviceOperateCbWithInfo cb);

private:
    void init();

    // error report
    void handleErrorAndRelase(GError *err);
    static void handleErrorAndRelease(CallbackProxy *proxy, bool result, GError *gerr, QString info = QString());

    // async callbacks
    static void mountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);
    static void unmountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);
    static void renameAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);
    static void ejectAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);
    static void powerOffAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);
    static void lockAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);
    static void unlockAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);

public:
    QString                 blkObjPath; // path of block device object
    UDisksClient            *client                 { nullptr };
    UDisksBlock             *blockHandler           { nullptr };
    UDisksFilesystem        *fileSystemHandler      { nullptr };
    UDisksDrive             *driveHandler           { nullptr };
    UDisksPartition         *partitionHandler       { nullptr };
    UDisksEncrypted         *encryptedHandler       { nullptr };
    UDisksPartitionTable    *partitionTabHandler    { nullptr };
    UDisksLoop              *loopHandler            { nullptr };
};
DFM_MOUNT_END_NS

#endif // DFMBLOCKDEVICE_P_H
