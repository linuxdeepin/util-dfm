// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dwatcher.h>

#include "private/dwatcher_p.h"

#include <QDebug>

USING_IO_NAMESPACE

/************************************************
 * DWatcherPrivate
 ***********************************************/

DWatcherPrivate::DWatcherPrivate(DWatcher *q)
    : q(q)
{
}

DWatcherPrivate::~DWatcherPrivate()
{
}

GFileMonitor *DWatcherPrivate::createMonitor(GFile *gfile, DWatcher::WatchType type)
{
    Q_UNUSED(type)

    if (!gfile) {
        error.setCode(DFMIOErrorCode(DFM_IO_ERROR_NOT_FOUND));
        return nullptr;
    }

    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GCancellable) cancel = g_cancellable_new();

    GFileMonitorFlags flags = GFileMonitorFlags(G_FILE_MONITOR_WATCH_MOUNTS | G_FILE_MONITOR_WATCH_MOVES);
    gmonitor = g_file_monitor(gfile, flags, cancel, &gerror);

    if (!gmonitor) {
        setErrorFromGError(gerror);

        return nullptr;
    }
    return gmonitor;
}

void DWatcherPrivate::setErrorFromGError(GError *gerror)
{
    if (!gerror)
        return error.setCode(DFMIOErrorCode(DFM_IO_ERROR_FAILED));
    error.setCode(DFMIOErrorCode(gerror->code));
    if (error.code() == DFMIOErrorCode::DFM_IO_ERROR_FAILED)
        error.setMessage(gerror->message);
}

void DWatcherPrivate::watchCallback(GFileMonitor *monitor, GFile *child, GFile *other,
                                    GFileMonitorEvent eventType, gpointer userData)
{
    Q_UNUSED(monitor);

    DWatcher *watcher = static_cast<DWatcher *>(userData);
    if (nullptr == watcher) {
        return;
    }

    QUrl childUrl = getUrl(child);
    QUrl otherUrl = getUrl(other);

    if (childUrl.path().startsWith("//"))
        childUrl.setPath(childUrl.path().mid(1));
    if (otherUrl.path().startsWith("//"))
        otherUrl.setPath(otherUrl.path().mid(1));

    switch (eventType) {
    case G_FILE_MONITOR_EVENT_CHANGED:
        watcher->fileChanged(childUrl);
        break;
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        watcher->fileDeleted(childUrl);
        break;
    case G_FILE_MONITOR_EVENT_CREATED:
        watcher->fileAdded(childUrl);
        break;
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        watcher->fileChanged(childUrl);
        break;
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
        break;
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
        watcher->fileDeleted(childUrl);
        break;
    case G_FILE_MONITOR_EVENT_MOVED_IN:
        watcher->fileAdded(childUrl);
        break;
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
        watcher->fileDeleted(childUrl);
        break;
    case G_FILE_MONITOR_EVENT_RENAMED:
        watcher->fileRenamed(childUrl, otherUrl);
        break;

    //case G_FILE_MONITOR_EVENT_MOVED:
    default:
        g_assert_not_reached();
        break;
    }
}

QUrl DWatcherPrivate::getUrl(GFile *file)
{
    if (!file)
        return QUrl();

    g_autofree gchar *path = g_file_get_path(file);
    QString strPath(path);
    if (!strPath.isEmpty() && strPath != '/') {
        return QUrl::fromLocalFile(strPath);
    } else {
        g_autofree gchar *uri = g_file_get_uri(file);
        return QUrl::fromUserInput(uri);
    }
}

DWatcher::DWatcher(const QUrl &uri, QObject *parent)
    : QObject(parent), d(new DWatcherPrivate(this))
{
    d->uri = uri;
}

/************************************************
 * DWatcher
 ***********************************************/

DWatcher::~DWatcher()
{
}

QUrl DWatcher::uri() const
{
    return d->uri;
}

void DWatcher::setTimeRate(int msec)
{
    d->timeRate = msec;
}

int DWatcher::timeRate() const
{
    return d->timeRate;
}

void DWatcher::setWatchType(DWatcher::WatchType type)
{
    d->type = type;
}

DWatcher::WatchType DWatcher::watchType() const
{
    return d->type;
}

bool DWatcher::running() const
{
    return d->gmonitor != nullptr;
}

bool DWatcher::start(int timeRate)
{
    // stop first
    stop();

    const QUrl &uri = this->uri();
    QString url = uri.url();
    if (uri.scheme() == "file" && uri.path() == "/")
        url.append("/");

    d->gfile = g_file_new_for_uri(url.toStdString().c_str());

    d->gmonitor = d->createMonitor(d->gfile, d->type);

    if (!d->gmonitor) {
        g_object_unref(d->gfile);
        d->gfile = nullptr;

        return false;
    }

    g_file_monitor_set_rate_limit(d->gmonitor, timeRate);

    g_signal_connect(d->gmonitor, "changed", G_CALLBACK(&DWatcherPrivate::watchCallback), this);

    return true;
}

bool DWatcher::stop()
{
    if (d->gmonitor) {
        g_file_monitor_cancel(d->gmonitor);
        g_object_unref(d->gmonitor);
        d->gmonitor = nullptr;
    }

    if (d->gfile) {
        g_object_unref(d->gfile);
        d->gfile = nullptr;
    }

    return true;
}

DFMIOError DWatcher::lastError() const
{
    return d->error;
}
