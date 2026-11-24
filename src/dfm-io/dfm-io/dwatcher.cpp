// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/dwatcher.h>
#include "utils/dlocalhelper.h"

#include "private/dwatcher_p.h"

#include <QDebug>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

#include <sys/stat.h>

USING_IO_NAMESPACE

namespace {

// 同一inode经不同路径（多挂载点/bind/符号链接）监视时，GLib的inotify后端按inode共享
// 内核watch描述符：任一监视器cancel会误删共享订阅，使其它路径的存活监视器永久失聪，
// 且残留的path桶会让后续新建监视器挂到死订阅上。注册表按(st_dev, st_ino)对活跃监视器
// 计数：仍有同inode其它活跃者时，停止方只寄存GFileMonitor而不cancel；最后一个活跃者
// 停止时才统一cancel并析构全部寄存的监视器，此时GLib状态自净。
// 并发停止同一inode的两个监视器时，寄存的监视器可能被并发方随doomed清空；因此在锁内
// 先断开changed回调再入寄存桶，保证停止方在交还监视器后不再触碰它，避免悬垂指针。
// 复用寄存监视器时除了watch类型，还必须限定"创建它时用的本地路径"完全一致：
// GLib的事件child路径是用建监视时那条路径字符串拼出来的（g_build_filename(dirname, name)），
// 而不是从inode反查。若把/home/u/x建的监视器交给/data/home/u/x（同inode）复用，
// 事件会以file:///home/u/x/...上报，上层按当前视图URL匹配不上，等同于监视器失效。
class MonitorKeepAliveRegistry
{
public:
    static MonitorKeepAliveRegistry &instance()
    {
        static MonitorKeepAliveRegistry registry;
        return registry;
    }

    void addActive(InodeKey key)
    {
        if (key.isNull())
            return;
        QMutexLocker locker(&mutex);
        ++refCounts[key];
    }

    // 停止时调用：同inode仍有其它活跃监视器时，在锁内断开回调并寄存monitor（不cancel）
    // 并返回true；否则清除计数，并通过doomed交还该inode全部寄存的监视器，由调用方在锁外
    // cancel析构（寄存桶已清空，且其中监视器的回调早已断开，并发停止方不再触碰它们）。
    bool retainOrPark(InodeKey key, GFileMonitor *monitor, DWatcher::WatchType type,
                      const QByteArray &path, gpointer userData, QList<GFileMonitor *> *doomed)
    {
        doomed->clear();
        if (key.isNull())
            return false;
        QMutexLocker locker(&mutex);
        const int remaining = refCounts.value(key) - 1;
        if (remaining > 0) {
            refCounts[key] = remaining;
            g_signal_handlers_disconnect_by_data(monitor, userData);
            parked[key].append(ParkedMonitor { monitor, type, path });
            return true;
        }
        refCounts.remove(key);
        for (const ParkedMonitor &parkedMonitor : parked.value(key))
            doomed->append(parkedMonitor.monitor);
        parked.remove(key);
        return false;
    }

    // start时取走本inode的寄存监视器复用（watch类型与建监视时的路径都必须一致）并计数，
    // 避免反复start/stop在兄弟监视器存活期间不断新建监视器导致寄存桶无限增长。取走与计数
    // 在同一把锁内，避免并发最终停止在两者之间cancel共享订阅、使复用者失聪。
    // 路径必须精确匹配：GLib上报的child路径由建监视时的路径字符串拼出，跨路径复用会
    // 让事件带着旧路径前缀（例如监视/data/...却上报file:///home/...），上层会丢弃它们。
    // 返回nullptr表示无可用寄存监视器。
    GFileMonitor *reviveParked(InodeKey key, DWatcher::WatchType type, const QByteArray &path)
    {
        if (key.isNull())
            return nullptr;
        QMutexLocker locker(&mutex);
        auto it = parked.find(key);
        if (it == parked.end())
            return nullptr;
        QList<ParkedMonitor> &list = it.value();
        for (int i = list.size() - 1; i >= 0; --i) {
            const ParkedMonitor &item = list.at(i);
            if (item.type == type && item.path == path) {
                GFileMonitor *monitor = list.takeAt(i).monitor;
                if (list.isEmpty())
                    parked.erase(it);
                ++refCounts[key];
                return monitor;
            }
        }
        return nullptr;
    }

private:
    struct ParkedMonitor
    {
        GFileMonitor *monitor { nullptr };
        DWatcher::WatchType type = DWatcher::WatchType::kAuto;
        QByteArray path;   // 建这个监视器时用的本地路径：GLib用它拼事件child路径
    };

    QMutex mutex;
    QHash<InodeKey, int> refCounts;
    QHash<InodeKey, QList<ParkedMonitor>> parked;
};

// 取GFile对应的本地路径字符串；GLib的inotify后端用的正是这个字符串来拼事件child路径，
// 因此它既是注册表复用监视器的判据，也是stat取inode的来源
QByteArray localPathOf(GFile *gfile)
{
    if (!gfile)
        return QByteArray();
    g_autofree gchar *path = g_file_get_path(gfile);
    return path ? QByteArray(path) : QByteArray();
}

// 从监视目标GFile取(st_dev, st_ino)编码key；非本地路径或stat失败返回null key（走直接cancel）
InodeKey inodeKeyOf(GFile *gfile)
{
    const QByteArray path = localPathOf(gfile);
    if (path.isEmpty())
        return InodeKey();
    struct stat st;
    if (::stat(path.constData(), &st) != 0)
        return InodeKey();
    return InodeKey { st.st_dev, st.st_ino };
}

}   // namespace

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
    if (!gfile) {
        error.setCode(DFMIOErrorCode(DFM_IO_ERROR_NOT_FOUND));
        return nullptr;
    }

    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GCancellable) cancel = g_cancellable_new();

    GFileMonitorFlags flags = GFileMonitorFlags(G_FILE_MONITOR_WATCH_MOUNTS | G_FILE_MONITOR_WATCH_MOVES);
    if (type == DWatcher::WatchType::kAuto) {
        gmonitor = g_file_monitor(gfile, flags, cancel, &gerror);
    } else if (type == DWatcher::WatchType::kDir) {
        gmonitor = g_file_monitor_directory(gfile, flags, cancel, &gerror);
    } else {
        gmonitor = g_file_monitor_file(gfile, flags, cancel, &gerror);
    }

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

    // case G_FILE_MONITOR_EVENT_MOVED:
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
    // bug-204961 监视根目录时，监视的是"//"路径。所以监视上来的文件创建就是"//path"。
    // 使用QUrl::fromLocalFile转换"//path"，转换出来的url就是"file:///0.0.0/path"。
    // 修改在路径前对"//"进行处理。处理完了在做后面的判断，如果路径错误重新使用uri.
    strPath.replace("//", "/");
    if (!strPath.isEmpty()) {
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
    stop();
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

    d->gfile = DLocalHelper::createGFile(url);
    d->inodeKey = inodeKeyOf(d->gfile);

    // 复用注册表中同inode、同watch类型且同路径的寄存监视器：避免反复start/stop在兄弟
    // 监视器存活期间不断新建监视器导致寄存桶无限增长。reviveParked在锁内完成取走与计数。
    d->gmonitor = MonitorKeepAliveRegistry::instance().reviveParked(d->inodeKey, d->type, localPathOf(d->gfile));
    if (d->gmonitor) {
        g_file_monitor_set_rate_limit(d->gmonitor, timeRate);
        g_signal_connect(d->gmonitor, "changed", G_CALLBACK(&DWatcherPrivate::watchCallback), this);
        return true;
    }

    d->gmonitor = d->createMonitor(d->gfile, d->type);

    if (!d->gmonitor) {
        g_object_unref(d->gfile);
        d->gfile = nullptr;
        d->inodeKey = InodeKey();

        return false;
    }

    g_file_monitor_set_rate_limit(d->gmonitor, timeRate);

    g_signal_connect(d->gmonitor, "changed", G_CALLBACK(&DWatcherPrivate::watchCallback), this);

    // key在start时缓存：stop时目标可能已被删除重建（inode变化），重stat会减错计数
    MonitorKeepAliveRegistry::instance().addActive(d->inodeKey);

    return true;
}

bool DWatcher::stop()
{
    if (d->gmonitor) {
        QList<GFileMonitor *> doomed;
        if (MonitorKeepAliveRegistry::instance().retainOrPark(d->inodeKey, d->gmonitor, d->type,
                                                             localPathOf(d->gfile), this, &doomed)) {
            // 同inode仍有其它路径在监视：不cancel（会误删GLib共享订阅使存活者永久失聪），
            // 回调已在锁内断开，监视器交由注册表持有
            d->gmonitor = nullptr;
        } else {
            g_file_monitor_cancel(d->gmonitor);
            g_object_unref(d->gmonitor);
            d->gmonitor = nullptr;
            for (GFileMonitor *monitor : doomed) {
                g_file_monitor_cancel(monitor);
                g_object_unref(monitor);
            }
            doomed.clear();
        }
    }

    if (d->gfile) {
        g_object_unref(d->gfile);
        d->gfile = nullptr;
    }

    d->inodeKey = InodeKey();

    return true;
}

DFMIOError DWatcher::lastError() const
{
    return d->error;
}
