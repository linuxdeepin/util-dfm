// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBLOCKMONITOR_H
#define DBLOCKMONITOR_H

#include <dfm-mount/base/ddevicemonitor.h>

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
