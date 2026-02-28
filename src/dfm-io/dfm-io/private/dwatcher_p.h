// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DWATCHER_P_H
#define DWATCHER_P_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfileinfo.h>
#include <dfm-io/dwatcher.h>

#include <QUrl>

#include <gio/gio.h>

BEGIN_IO_NAMESPACE

class DWatcher;
class DWatcherPrivate
{
public:
    explicit DWatcherPrivate(DWatcher *q);
    virtual ~DWatcherPrivate();
    GFileMonitor *createMonitor(GFile *gfile, DWatcher::WatchType type);
    void setErrorFromGError(GError *gerror);

    static void watchCallback(GFileMonitor *monitor, GFile *child, GFile *other,
                              GFileMonitorEvent eventType, gpointer userData);
    static QUrl getUrl(GFile *file);

public:
    DWatcher *q { nullptr };
    GFileMonitor *gmonitor { nullptr };
    GFile *gfile { nullptr };

    int timeRate { 200 };
    DWatcher::WatchType type = DWatcher::WatchType::kAuto;
    QUrl uri;
    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DWATCHER_P_H
