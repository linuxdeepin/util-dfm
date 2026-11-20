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

#include <sys/types.h>

BEGIN_IO_NAMESPACE

// (st_dev, st_ino)完整保留的inode标识，作为监视器注册表的key；避免截断导致
// 不同inode撞key（如btrfs的64位inode号）而延迟清理寄存的监视器。
struct InodeKey
{
    dev_t dev { 0 };
    ino_t ino { 0 };

    bool isNull() const { return dev == 0 && ino == 0; }
    bool operator==(const InodeKey &other) const { return dev == other.dev && ino == other.ino; }
};

inline uint qHash(const InodeKey &key, uint seed = 0)
{
    return ::qHash(static_cast<quint64>(key.dev), seed)
            ^ (::qHash(static_cast<quint64>(key.ino), seed) << 1);
}

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
    InodeKey inodeKey;   // start时缓存的(st_dev, st_ino)，保证stop与start对同一inode计数

    int timeRate { 200 };
    DWatcher::WatchType type = DWatcher::WatchType::kAuto;
    QUrl uri;
    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DWATCHER_P_H
