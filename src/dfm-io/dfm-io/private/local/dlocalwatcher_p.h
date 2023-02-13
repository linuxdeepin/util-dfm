// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DLOCALWATCHER_P_H
#define DLOCALWATCHER_P_H

#include "dfmio_global.h"

#include "core/dwatcher_p.h"

#include "gio/gio.h"

BEGIN_IO_NAMESPACE

class DLocalWatcher;

class DLocalWatcherPrivate
{
public:
    explicit DLocalWatcherPrivate(DLocalWatcher *q);
    ~DLocalWatcherPrivate();

    void setWatchType(DWatcher::WatchType type);
    DWatcher::WatchType watchType() const;
    bool start(int timeRate);
    bool stop();
    bool running() const;

    DFMIOError lastError();
    void setErrorFromGError(GError *gerror);

    static void watchCallback(GFileMonitor *gmonitor, GFile *child, GFile *other, GFileMonitorEvent event_type, gpointer user_data);

private:
    GFileMonitor *createMonitor(GFile *gfile, DWatcher::WatchType type);

public:
    GFileMonitor *gmonitor = nullptr;
    GFile *gfile = nullptr;
    DWatcher::WatchType type = DWatcher::WatchType::kAuto;

    DFMIOError error;

    DLocalWatcher *q = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALWATCHER_P_H
