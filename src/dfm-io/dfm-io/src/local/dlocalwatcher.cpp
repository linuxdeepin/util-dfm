/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#include "local/dlocalwatcher.h"
#include "local/dlocalwatcher_p.h"

#include <gio/gio.h>

#include <QDebug>

USING_IO_NAMESPACE

DLocalWatcherPrivate::DLocalWatcherPrivate(DLocalWatcher *q)
    : q_ptr(q)
{

}

DLocalWatcherPrivate::~DLocalWatcherPrivate()
{

}

void DLocalWatcherPrivate::setWatchType(DWatcher::WatchType type)
{
    this->type = type;
}

DWatcher::WatchType DLocalWatcherPrivate::watchType() const
{
    return this->type;
}

bool DLocalWatcherPrivate::start(int timeRate)
{
    Q_Q(DLocalWatcher);

    // stop the last monitor.

    if (monitor)
        g_object_unref(monitor);
    monitor = nullptr;

    GError *error = nullptr;
    GFile *file = nullptr;

    // stop first
    if (!stop()) {
        //
    }

    const QUrl &uri = q->uri();
    const QString &fname = uri.url();

    file = g_file_new_for_uri(fname.toLocal8Bit().data());

    if (type == DWatcher::WatchType::AUTO) {
        GFileInfo *info;
        guint32 fileType;

        info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, nullptr, &error);
        if (!info)
            goto err;

        fileType = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE);
        type = (fileType == G_FILE_TYPE_DIRECTORY) ? DWatcher::WatchType::DIR : DWatcher::WatchType::FILE;
    }

    if (type == DWatcher::WatchType::DIR)
        monitor = g_file_monitor_directory (file, G_FILE_MONITOR_WATCH_MOVES, nullptr, &error);
      else
        monitor = g_file_monitor (file, G_FILE_MONITOR_WATCH_MOVES, nullptr, &error);

    if (!monitor)
        goto err;

    g_file_monitor_set_rate_limit(monitor, timeRate);

    g_signal_connect(monitor, "changed", G_CALLBACK(&DLocalWatcherPrivate::watchCallback), this);
    //g_object_unref(file);
    return true;

err:
    qInfo() << "error:" << error->message;
    g_error_free(error);
    g_object_unref(file);

    return false;
}
bool DLocalWatcherPrivate::stop()
{
    if (monitor) {
        if (!g_file_monitor_cancel(monitor)) {
            qInfo() << "cancel file monitor failed.";
        }
        g_object_unref(monitor);
        monitor = nullptr;
    }

    return true;
}

bool DLocalWatcherPrivate::running() const
{
    return monitor != nullptr;
}

void DLocalWatcherPrivate::watchCallback(GFileMonitor *monitor,
                                       GFile *child,
                                       GFile *other,
                                       GFileMonitorEvent event_type,
                                       gpointer user_data)
{
    Q_UNUSED(monitor);

    DLocalWatcher *watcher = static_cast<DLocalWatcher*>(user_data);
    if (watcher == nullptr) {
        return;
    }

    gchar *child_str;
    gchar *other_str;

    child_str = g_file_get_uri(child);

    if (other) {
        other_str = g_file_get_uri(other);
    } else {
        other_str = g_strdup("(none)");
    }

    switch (event_type) {
    case G_FILE_MONITOR_EVENT_CHANGED:
        // g_print ("%s: changed", child_str);
        Q_EMIT watcher->fileChanged(QUrl(child_str), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
        // g_print ("%s: changes done", child_str);
        Q_EMIT watcher->fileChanged(QUrl(child_str), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        // g_print ("%s: deleted", child_str);
        Q_EMIT watcher->fileDeleted(QUrl(child_str), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_CREATED:
        // g_print ("%s: created", child_str);
        //QMetaObject::invokeMethod(watcher, "fileAdded", Qt::QueuedConnection, Q_ARG(QUrl, QUrl(child_str)), Q_ARG(DFileInfo, DFileInfo()));
        Q_EMIT watcher->fileAdded(QUrl(child_str), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        // g_print ("%s: attributes changed", child_str);
        Q_EMIT watcher->fileChanged(QUrl(child_str), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
        // g_print ("%s: pre-unmount", child_str);
        break;
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
        // g_print ("%s: unmounted", child_str);
        break;
    case G_FILE_MONITOR_EVENT_MOVED_IN:
        // g_print ("%s: moved in", child_str);
        // if (other)
        //     g_print (" (from %s)", other_str);
        Q_EMIT watcher->fileAdded(QUrl(child_str), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
        // g_print ("%s: moved out", child_str);
        // if (other)
        //     g_print (" (to %s)", other_str);
        Q_EMIT watcher->fileDeleted(QUrl(child_str), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_RENAMED:
        // g_print ("%s: renamed to %s\n", child_str, other_str);
        Q_EMIT watcher->fileDeleted(QUrl(child_str), DFileInfo());
        Q_EMIT watcher->fileAdded(QUrl(other_str), DFileInfo());
        break;

    case G_FILE_MONITOR_EVENT_MOVED:
    default:
        g_assert_not_reached();
        break;
    }

    g_free(child_str);
    g_free(other_str);
}

DLocalWatcher::DLocalWatcher(const QUrl &uri, QObject *parent)
    : DWatcher(uri, parent)
    , d_ptr(new DLocalWatcherPrivate(this))
{
    registerSetWatchType(std::bind(&DLocalWatcher::setWatchType, this, std::placeholders::_1));
    registerWatchType(std::bind(&DLocalWatcher::watchType, this));
    registerRunning(std::bind(&DLocalWatcher::running, this));
    registerStart(std::bind(&DLocalWatcher::start, this, std::placeholders::_1));
    registerStop(std::bind(&DLocalWatcher::stop, this));
}

DLocalWatcher::~DLocalWatcher()
{
    Q_D(DLocalWatcher);
    d->stop();
}

void DLocalWatcher::setWatchType(DWatcher::WatchType type)
{
    Q_D(DLocalWatcher);
    d->setWatchType(type);
}

DWatcher::WatchType DLocalWatcher::watchType() const
{
    Q_D(const DLocalWatcher);
    return d->watchType();
}

bool DLocalWatcher::running() const
{
    Q_D(const DLocalWatcher);

    return d->running();
}

bool DLocalWatcher::start(int timeRate)
{
    Q_D(DLocalWatcher);

    return d->start(timeRate);
}

bool DLocalWatcher::stop()
{
    Q_D(DLocalWatcher);

    return d->stop();
}
