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
#ifndef DFMABSTRACTDEVICE_H
#define DFMABSTRACTDEVICE_H

#include "dfmmount_global.h"
#include "dfmdeviceinterface.h"

#include <QObject>

DFM_MOUNT_BEGIN_NS
class DFMAbstractDevice: public QObject
{
    Q_OBJECT
public:
    explicit DFMAbstractDevice(QObject *parent = nullptr);
//    virtual ~DFMAbstractDevice();

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
    long sizeFree();
    long sizeUsage();

    int deviceType();

Q_SIGNALS:
    void mounted(const QUrl &mountPoint);
    void unmounted();
    void renamed(const QString &newName);

protected:
    DfmDeviceInterface iface;
};
DFM_MOUNT_END_NS

#endif // DFMABSTRACTDEVICE_H
