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
#ifndef DBLOCKMONITOR_H
#define DBLOCKMONITOR_H

#include "base/ddevicemonitor.h"

#include <QObject>

DFM_MOUNT_BEGIN_NS

class DBlockMonitorPrivate;
class DBlockMonitor final : public DDeviceMonitor
{
    Q_OBJECT

public:
    DBlockMonitor(QObject *parent = nullptr);
    ~DBlockMonitor();

    QStringList resolveDevice(const QVariantMap &devspec, const QVariantMap &opts);
    QStringList resolveDeviceNode(const QString &node, const QVariantMap &opts);
    QStringList resolveDeviceFromDrive(const QString &drvObjPath);

Q_SIGNALS:
    void driveAdded(const QString &drvObjPath);
    void driveRemoved(const QString &drvObjPath);
    void fileSystemAdded(const QString &blkObjPath);
    void fileSystemRemoved(const QString &blkObjPath);
    void blockLocked(const QString &blkObjPath);
    void blockUnlocked(const QString &blkObjPath, const QString &clearDevObjPath);
};
DFM_MOUNT_END_NS

#endif   // DBLOCKMONITOR_H
