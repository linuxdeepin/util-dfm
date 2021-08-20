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

DFM_MOUNT_BEGIN_NS
class DFMBlockDevicePrivate {
public:
    DFMBlockDevicePrivate(DFMBlockDevice *qq, const QString &dev);

    QUrl mount(const QVariantMap &opts);
    void mountAsync(const QVariantMap &opts);
    bool unmount();
    void unmountAsync();
    bool rename(const QString &newName);
    void renameAsync(const QString &newName);
    QUrl accessPoint();
    QUrl mountPoint();
    QString fileSystem();
    long sizeTotal();
    long sizeUsage();
    long sizeFree();
    int deviceType();

    bool eject();
    bool powerOff();

public:
    QString devDesc; // descriptor of device

private:
    DFMBlockDevice* q_ptr;
    Q_DECLARE_PUBLIC(DFMBlockDevice)
};
DFM_MOUNT_END_NS

#endif // DFMBLOCKDEVICE_P_H
