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

DFM_MOUNT_BEGIN_NS

class DFMBlockDevicePrivate final: public DFMDevicePrivate
{
    Q_DECLARE_PUBLIC(DFMBlockDevice)

public:
    DFMBlockDevicePrivate();

    QString path() const DFM_MNT_OVERRIDE;
    QUrl mount(const QVariantMap &opts) DFM_MNT_OVERRIDE;
    void mountAsync(const QVariantMap &opts) DFM_MNT_OVERRIDE;
    bool unmount() DFM_MNT_OVERRIDE;
    void unmountAsync() DFM_MNT_OVERRIDE;
    bool rename(const QString &newName) DFM_MNT_OVERRIDE;
    void renameAsync(const QString &newName) DFM_MNT_OVERRIDE;
    QUrl accessPoint() const DFM_MNT_OVERRIDE;
    QUrl mountPoint() const DFM_MNT_OVERRIDE;
    QString fileSystem() const DFM_MNT_OVERRIDE;
    long sizeTotal() const DFM_MNT_OVERRIDE;
    long sizeUsage() const DFM_MNT_OVERRIDE;
    long sizeFree() const DFM_MNT_OVERRIDE;
    DeviceType deviceType() const DFM_MNT_OVERRIDE;
    QVariant getProperty(Property name) const DFM_MNT_OVERRIDE;

    QVariant getBlockProperty(Property name) const;
    QVariant getDriveProperty(Property name) const;
    QVariant getFileSystemProperty(Property name) const;
    QVariant getPartitionProperty(Property name) const;

    bool eject();
    bool powerOff();

private:
    QString charToQString(char *tmp) const;
    QStringList charToQStringList(char **tmp) const;

public:
    QString devDesc; // descriptor of device
    UDisksBlock *blockHandler = nullptr;
    UDisksFilesystem *fileSystemHandler = nullptr;
    UDisksDrive *driveHandler = nullptr;
    UDisksPartition *partitionHandler = nullptr;
};
DFM_MOUNT_END_NS

#endif // DFMBLOCKDEVICE_P_H
