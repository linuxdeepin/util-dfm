// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dwatcher.h>

#include "private/dwatcher_p.h"

#include <QDebug>

#include <sys/inotify.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
    if (gerror->domain != G_IO_ERROR || gerror->message) {
        error.setCode(DFMIOErrorCode::DFM_ERROR_OTHER_DOMAIN);
        error.setMessage(gerror->message);
    }
}

bool DWatcherPrivate::startProxy()
{
    if (!proxy)
        proxy = new DLocalWatcherProxy(q);

    return proxy->start();
}

void DWatcherPrivate::watchCallback(GFileMonitor *monitor, GFile *child, GFile *other,
                                    GFileMonitorEvent eventType, gpointer userData)
{
    Q_UNUSED(monitor);

    DWatcher *watcher = static_cast<DWatcher *>(userData);
    if (nullptr == watcher) {
        return;
    }

    QUrl childUrl;
    QUrl otherUrl;

    g_autofree gchar *childStr = g_file_get_path(child);
    if (childStr != nullptr && *childStr != '/') {
        childUrl = QUrl::fromLocalFile(childStr);
    } else {
        g_autofree gchar *uri = g_file_get_uri(child);
        childUrl = QUrl::fromUserInput(uri);
    }

    if (other) {
        g_autofree gchar *otherStr = g_file_get_path(other);
        if (otherStr != nullptr && *otherStr != '/') {
            otherUrl = QUrl::fromLocalFile(otherStr);
        } else {
            g_autofree gchar *uri = g_file_get_uri(other);
            otherUrl = QUrl::fromUserInput(uri);
        }
    }

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
    if (d->proxy)
        return d->proxy->running();

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

        return d->startProxy();
    }

    g_file_monitor_set_rate_limit(d->gmonitor, timeRate);

    g_signal_connect(d->gmonitor, "changed", G_CALLBACK(&DWatcherPrivate::watchCallback), this);

    return true;
}

bool DWatcher::stop()
{
    if (d->proxy) {
        d->proxy->stop();
    } else {
        if (d->gmonitor) {
            g_file_monitor_cancel(d->gmonitor);
            g_object_unref(d->gmonitor);
            d->gmonitor = nullptr;
        }
        if (d->gfile) {
            g_object_unref(d->gfile);
            d->gfile = nullptr;
        }
    }

    return true;
}

DFMIOError DWatcher::lastError() const
{
    return d->error;
}

/************************************************
 * DLocalWatcherProxy
 ***********************************************/

DLocalWatcherProxy::DLocalWatcherProxy(DWatcher *qq)
    : QObject(qq),
      q(qq),
      monitorFile(qq->uri().path())
{
#ifdef IN_CLOEXEC
    inotifyFd = inotify_init1(IN_CLOEXEC);
#endif

    if (inotifyFd == -1)
        inotifyFd = inotify_init();

    if (inotifyFd != -1) {
        notifier = new QSocketNotifier(inotifyFd, QSocketNotifier::Read, this);
        fcntl(inotifyFd, F_SETFD, FD_CLOEXEC);
        connect(notifier, SIGNAL(activated(int)), this, SLOT(onNotifierActived()));
        inited = true;
    }
}

DLocalWatcherProxy::~DLocalWatcherProxy()
{
    if (inited) {
        notifier->setEnabled(false);
        stop();
        ::close(inotifyFd);
    }
}

bool DLocalWatcherProxy::start()
{
    if (!inited || !q->uri().isLocalFile())
        return false;

    stop();

    g_autoptr(GCancellable) cancel = g_cancellable_new();
    g_autoptr(GFile) gfile = g_file_new_for_path(monitorFile.toLocal8Bit().data());
    bool isDir = (g_file_query_file_type(gfile, G_FILE_QUERY_INFO_NONE, cancel) == G_FILE_TYPE_DIRECTORY);

    watchId = inotify_add_watch(inotifyFd,
                                monitorFile.toLocal8Bit(),
                                (isDir ? (0
                                          | IN_ATTRIB
                                          | IN_MOVE
                                          | IN_MOVE_SELF
                                          | IN_CREATE
                                          | IN_DELETE
                                          | IN_DELETE_SELF
                                          | IN_MODIFY
                                          | IN_UNMOUNT)
                                       : (0
                                          | IN_ATTRIB
                                          | IN_CLOSE_WRITE
                                          | IN_MODIFY
                                          | IN_MOVE
                                          | IN_MOVE_SELF
                                          | IN_DELETE_SELF)));

    if (watchId < 0)
        return false;

    type = isDir ? DWatcher::WatchType::kDir : DWatcher::WatchType::kFile;
    return true;
}

bool DLocalWatcherProxy::stop()
{
    if (inited && watchId >= 0) {
        inotify_rm_watch(inotifyFd, watchId);
        watchId = -1;
        return true;
    }

    return false;
}

bool DLocalWatcherProxy::running()
{
    return inited;
}

void DLocalWatcherProxy::onNotifierActived()
{
    int buffSize = 0;
    if (ioctl(inotifyFd, FIONREAD, (char *)&buffSize) < 0 || buffSize == 0)
        return;

    QVarLengthArray<char, 4096> buffer(buffSize);
    buffSize = static_cast<int>(read(inotifyFd, buffer.data(), static_cast<size_t>(buffSize)));
    char *at = buffer.data();
    char *const end = at + buffSize;

    QList<inotify_event *> eventList;
    /// only save event: IN_MOVE_TO
    QMultiMap<uint32_t, QString> cookieToFilePath;
    QMultiMap<uint32_t, QString> cookieToFileName;
    QSet<uint32_t> hasMoveFromByCookie;

    while (at < end) {
        inotify_event *event = reinterpret_cast<inotify_event *>(at);
        at += sizeof(inotify_event) + event->len;

        if (!(event->mask & IN_MOVED_TO) || !hasMoveFromByCookie.contains(event->cookie))
            eventList.append(event);

        if (event->mask & IN_MOVED_TO) {
            cookieToFilePath.insert(event->cookie, monitorFile);
            cookieToFileName.insert(event->cookie, QString::fromUtf8(event->name));
        }

        if (event->mask & IN_MOVED_FROM)
            hasMoveFromByCookie << event->cookie;
    }

    QList<inotify_event *>::const_iterator it = eventList.constBegin();
    while (it != eventList.constEnd()) {
        const inotify_event &event = **it;
        ++it;

        int id = event.wd;
        const QString &name = QString::fromUtf8(event.name, static_cast<int>(strlen(event.name)));
        if ((event.mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT)) != 0) {
            if (id == watchId)
                stop();

            do {
                if (event.mask & IN_MOVE_SELF) {
                    if (name.isEmpty())
                        break;
                }

                q->fileDeleted(QUrl::fromLocalFile(monitorFile));
            } while (false);
        }

        QString filePath = monitorFile;
        if (DWatcher::WatchType::kDir == type) {
            if (filePath.endsWith("/"))
                filePath = filePath + name;
            else
                filePath = filePath + "/" + name;
        }

        // watcher is invaild when file deleted laster than create
        if (event.mask & IN_CREATE) {
            if (name.isEmpty()) {
                stop();
                start();
            } else if (filePath == monitorFile) {
                stop();
                start();
            }

            q->fileAdded(QUrl::fromLocalFile(filePath));
        }

        if (event.mask & IN_DELETE)
            q->fileDeleted(QUrl::fromLocalFile(filePath));

        if (event.mask & IN_MOVED_FROM) {
            const QString toName = cookieToFileName.value(event.cookie);
            if (cookieToFilePath.values(event.cookie).empty()) {
                q->fileDeleted(QUrl::fromLocalFile(filePath));
            } else {
                for (QString &toPath : cookieToFilePath.values(event.cookie)) {
                    QUrl toUrl = QUrl::fromLocalFile(toPath + "/" + toName);
                    q->fileRenamed(QUrl::fromLocalFile(filePath), toUrl);
                }
            }
        }

        if (event.mask & IN_MOVED_TO) {
            if (!hasMoveFromByCookie.contains(event.cookie)) {
                QUrl toUrl = QUrl::fromLocalFile(monitorFile + "/" + name);
                q->fileAdded(toUrl);
            }
        }

        if (event.mask & IN_ATTRIB || event.mask & IN_MODIFY)
            q->fileChanged(QUrl::fromLocalFile(filePath));
    }
}
