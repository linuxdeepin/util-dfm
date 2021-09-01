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
#ifndef DFMPROTOCOLDEVICE_P_H
#define DFMPROTOCOLDEVICE_P_H

#include "dfmprotocoldevice.h"
#include "udisks/udisks-generated.h"

DFM_MOUNT_BEGIN_NS
class DFMProtocolDevicePrivate final {
public:
    DFMProtocolDevicePrivate(DFMProtocolDevice *qq, const QString &dev);

    QString path() const;
    QUrl mount(const QVariantMap &opts);
    void mountAsync(const QVariantMap &opts);
    bool unmount();
    void unmountAsync();
    bool rename(const QString &newName);
    void renameAsync(const QString &newName);
    QUrl accessPoint() const;
    QUrl mountPoint() const;
    QString fileSystem() const;
    long sizeTotal() const;
    long sizeUsage() const;
    long sizeFree() const;
    DeviceType deviceType() const;

    bool eject();
    bool powerOff();

public:
    QString devDesc; // descriptor of device

private:
    DFMProtocolDevice* q_ptr;
    Q_DECLARE_PUBLIC(DFMProtocolDevice)
};
DFM_MOUNT_END_NS

#endif // DFMPROTOCOLDEVICE_P_H
