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
#ifndef DFMDEVICEINTERFACE_H
#define DFMDEVICEINTERFACE_H

#include "dfmmount_global.h"

#include <QObject>
#include <QUrl>

#include <functional>

using namespace std;
DFM_MOUNT_BEGIN_NS

struct DfmDeviceInterface {
    function<QUrl (const QVariantMap &)> mount;
    function<void (const QVariantMap &)> mountAsync;

    function<bool ()> unmount;
    function<void ()> unmountAsync;

    function<bool (const QString &)> rename;
    function<void (const QString &)> renameAsync;

    function<QUrl ()> accessPoint;
    function<QUrl ()> mountPoint;

    function<QString ()> fileSystem;

    function<long ()> sizeTotal;
    function<long ()> sizeUsage;
    function<long ()> sizeFree;

    function<int ()> deviceType;
};

DFM_MOUNT_END_NS

#endif // DFMDEVICEINTERFACE_H
