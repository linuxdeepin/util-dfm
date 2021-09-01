/*
 * Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
 *
 * Author:     xushitong <xushitong@uniontech.com>
 *
 * Maintainer: xushitong <xushitong@uniontech.com>
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
#ifndef DFMDEVICEMANAGER_H
#define DFMDEVICEMANAGER_H

#include "base/dfmmount_global.h"
#include "base/dfmmountdefines.h"

#include <QObject>

DFM_MOUNT_BEGIN_NS

class DFMDeviceManagerPrivate;
class DFMDeviceManager final : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DFMDeviceManager)

public:
    DFMDeviceManager(QObject *parent = nullptr);
    ~DFMDeviceManager();
};

DFM_MOUNT_END_NS

#endif // DFMDEVICEMANAGER_H
