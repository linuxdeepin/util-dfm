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
    : q(q)
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
    // stop first
    stop();

    const QUrl &uri = q->uri();
    const QString &url = uri.url();

    gfile = g_file_new_for_uri(url.toLocal8Bit().data());

    gmonitor = createMonitor(gfile, type);

    if (!gmonitor) {
        g_object_unref(gfile);
        gfile = nullptr;

        return false;
    }

    g_file_monitor_set_rate_limit(gmonitor, timeRate);

    g_signal_connect(gmonitor, "changed", G_CALLBACK(&DLocalWatcherPrivate::watchCallback), q);

    return true;
}
bool DLocalWatcherPrivate::stop()
{
    if (gmonitor) {
        if (!g_file_monitor_cancel(gmonitor)) {
            qInfo() << "cancel file monitor failed.";
        }
        g_object_unref(gmonitor);
        gmonitor = nullptr;
    }
    if (gfile) {
        g_object_unref(gfile);
        gfile = nullptr;
    }

    return true;
}

bool DLocalWatcherPrivate::running() const
{
    return gmonitor != nullptr;
}

DFMIOError DLocalWatcherPrivate::lastError()
{
    return error;
}

void DLocalWatcherPrivate::setErrorInfo(GError *gerror)
{
    error.setCode(DFMIOErrorCode(gerror->code));

    qWarning() << QString::fromLocal8Bit(gerror->message);
}

void DLocalWatcherPrivate::watchCallback(GFileMonitor *monitor,
                                         GFile *child,
                                         GFile *other,
                                         GFileMonitorEvent event_type,
                                         gpointer user_data)
{
    Q_UNUSED(monitor);

    DLocalWatcher *watcher = static_cast<DLocalWatcher *>(user_data);
    if (nullptr == watcher) {
        return;
    }

    QString childUrl;
    QString otherUrl;

    g_autofree gchar *child_str = g_file_get_uri(child);
    childUrl = QString::fromLocal8Bit(child_str);
    if (other) {
        g_autofree gchar *other_str = g_file_get_uri(other);
        otherUrl = QString::fromLocal8Bit(other_str);
    }

    switch (event_type) {
    case G_FILE_MONITOR_EVENT_CHANGED:
        watcher->fileChanged(QUrl(childUrl), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
        //watcher->fileChanged(QUrl(childUrl), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_DELETED:
        watcher->fileDeleted(QUrl(childUrl), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_CREATED:
        watcher->fileAdded(QUrl(childUrl), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
        watcher->fileChanged(QUrl(childUrl), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
        break;
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
        break;
    case G_FILE_MONITOR_EVENT_MOVED_IN:
        watcher->fileAdded(QUrl(childUrl), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
        watcher->fileDeleted(QUrl(childUrl), DFileInfo());
        break;
    case G_FILE_MONITOR_EVENT_RENAMED:
        watcher->fileRenamed(QUrl(childUrl), QUrl(otherUrl));
        break;

    //case G_FILE_MONITOR_EVENT_MOVED:
    default:
        g_assert_not_reached();
        break;
    }
}

DWatcher::WatchType DLocalWatcherPrivate::transWatcherType(GFile *gfile, bool *ok)
{
    DWatcher::WatchType retType = DWatcher::WatchType::kAuto;

    if (!gfile)
        return retType;

    g_autoptr(GFileInfo) gfileinfo = nullptr;
    guint32 fileType;
    g_autoptr(GError) gerror = nullptr;

    gfileinfo = g_file_query_info(gfile, G_FILE_ATTRIBUTE_STANDARD_TYPE, G_FILE_QUERY_INFO_NONE, nullptr, &gerror);
    if (!gfileinfo) {
        setErrorInfo(gerror);

        return retType;
    }

    fileType = g_file_info_get_attribute_uint32(gfileinfo, G_FILE_ATTRIBUTE_STANDARD_TYPE);

    retType = (fileType == G_FILE_TYPE_DIRECTORY) ? DWatcher::WatchType::kDir : DWatcher::WatchType::kFile;

    if (ok)
        *ok = true;
    return retType;
}

GFileMonitor *DLocalWatcherPrivate::createMonitor(GFile *gfile, DWatcher::WatchType type)
{
    if (!gfile)
        return nullptr;
    if (type == DWatcher::WatchType::kAuto) {
        bool ok = false;
        type = transWatcherType(gfile, &ok);
        if (!ok)
            return nullptr;
    }

    g_autoptr(GError) gerror = nullptr;
    if (type == DWatcher::WatchType::kDir)
        gmonitor = g_file_monitor_directory(gfile, G_FILE_MONITOR_WATCH_MOVES, nullptr, &gerror);
    else
        gmonitor = g_file_monitor(gfile, G_FILE_MONITOR_WATCH_MOVES, nullptr, &gerror);

    if (!gmonitor) {
        setErrorInfo(gerror);

        return nullptr;
    }
    return gmonitor;
}

DLocalWatcher::DLocalWatcher(const QUrl &uri, QObject *parent)
    : DWatcher(uri, parent), d(new DLocalWatcherPrivate(this))
{
    registerSetWatchType(std::bind(&DLocalWatcher::setWatchType, this, std::placeholders::_1));
    registerWatchType(std::bind(&DLocalWatcher::watchType, this));
    registerRunning(std::bind(&DLocalWatcher::running, this));
    registerStart(std::bind(&DLocalWatcher::start, this, std::placeholders::_1));
    registerStop(std::bind(&DLocalWatcher::stop, this));
    registerLastError(std::bind(&DLocalWatcher::lastError, this));
}

DLocalWatcher::~DLocalWatcher()
{
    d->stop();
}

void DLocalWatcher::setWatchType(DWatcher::WatchType type)
{
    d->setWatchType(type);
}

DWatcher::WatchType DLocalWatcher::watchType() const
{
    return d->watchType();
}

bool DLocalWatcher::running() const
{
    return d->running();
}

bool DLocalWatcher::start(int timeRate /*= 200*/)
{
    return d->start(timeRate);
}

bool DLocalWatcher::stop()
{
    return d->stop();
}

DFMIOError DLocalWatcher::lastError() const
{
    return d->lastError();
}
