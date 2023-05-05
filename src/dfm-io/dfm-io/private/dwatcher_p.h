// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DWATCHER_P_H
#define DWATCHER_P_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfileinfo.h>
#include <dfm-io/dwatcher.h>

#include <QUrl>
#include <QSocketNotifier>

#include <gio/gio.h>

BEGIN_IO_NAMESPACE

class DWatcher;
class DLocalWatcherProxy;

class DWatcherPrivate
{
public:
    explicit DWatcherPrivate(DWatcher *q);
    virtual ~DWatcherPrivate();
    GFileMonitor *createMonitor(GFile *gfile, DWatcher::WatchType type);
    void setErrorFromGError(GError *gerror);
    bool startProxy();

    static void watchCallback(GFileMonitor *monitor, GFile *child, GFile *other,
                              GFileMonitorEvent eventType, gpointer userData);
    static QUrl getUrl(GFile *file);

public:
    DWatcher *q { nullptr };
    GFileMonitor *gmonitor { nullptr };
    DLocalWatcherProxy *proxy { nullptr };
    GFile *gfile { nullptr };

    int timeRate { 200 };
    DWatcher::WatchType type = DWatcher::WatchType::kAuto;
    QUrl uri;
    DFMIOError error;
};

class DLocalWatcherProxy : public QObject
{
    Q_OBJECT
public:
    explicit DLocalWatcherProxy(DWatcher *qq);
    virtual ~DLocalWatcherProxy();

    bool start();
    bool stop();
    bool running();

private Q_SLOTS:
    void onNotifierActived();

private:
    DWatcher *q { nullptr };

    QString monitorFile;
    DWatcher::WatchType type { DWatcher::WatchType::kAuto };
    int watchId { -1 };
    bool inited { false };
    int inotifyFd { -1 };
    QSocketNotifier *notifier { nullptr };
};

END_IO_NAMESPACE

#endif   // DWATCHER_P_H
