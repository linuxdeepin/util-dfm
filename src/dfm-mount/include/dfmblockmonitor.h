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
#ifndef DFMBLOCKMONITOR_H
#define DFMBLOCKMONITOR_H

#include "base/dfmmonitor.h"

#include <QObject>

DFM_MOUNT_BEGIN_NS

class DFMBlockMonitorPrivate;
class DFMBlockMonitor final : public DFMMonitor
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DFMBlockMonitor)

public:
    DFMBlockMonitor(QObject *parent = nullptr);
    ~DFMBlockMonitor();

    QStringList resolveDevice(const QVariantMap &devspec, const QVariantMap &opts);
    QStringList resolveDeviceNode(const QString &node, const QVariantMap &opts);
    QStringList resolveDeviceFromDrive(const QString &drvObjPath);

Q_SIGNALS:
    void driveAdded(const QString &drvObjPath);
    void driveRemoved(const QString &drvObjPath);
    void fileSystemAdded(const QString &blkObjPath);
    void fileSystemRemoved(const QString &blkObjPath);
};
DFM_MOUNT_END_NS

#endif // DFMBLOCKMONITOR_H
