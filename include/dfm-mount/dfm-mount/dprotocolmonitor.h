// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DPROTOCOLMONITOR_H
#define DPROTOCOLMONITOR_H

#include <dfm-mount/base/ddevicemonitor.h>

#include <QObject>

DFM_MOUNT_BEGIN_NS
class DProtocolMonitorPrivate;
class DProtocolMonitor : public DDeviceMonitor
{
    Q_OBJECT

public:
    DProtocolMonitor(QObject *parent = nullptr);
    ~DProtocolMonitor();
};
DFM_MOUNT_END_NS

#endif   // DBLOCKMONITOR_H
