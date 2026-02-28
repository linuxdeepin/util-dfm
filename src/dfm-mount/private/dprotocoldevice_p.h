// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DPROTOCOLDEVICE_P_H
#define DPROTOCOLDEVICE_P_H

#include <dfm-mount/base/dmount_global.h>
#include <dfm-mount/dprotocoldevice.h>

#include "private/ddevice_p.h"

#include <QMutex>
#include <QMutexLocker>
#include <QEventLoop>
#include <QTimer>

extern "C" {
#include <gio/gio.h>
}

DFM_MOUNT_BEGIN_NS
class ASyncToSyncHelper
{
    Q_DISABLE_COPY(ASyncToSyncHelper)

public:
    enum {
        NoError,
        Failed,
        Timeout,
    };
    explicit ASyncToSyncHelper(int timeout);
    ~ASyncToSyncHelper();

    inline QVariant result() { return ret; }
    inline void setResult(QVariant result) { ret = result; }
    inline int exec()
    {
        timer->start();
        return blocker->exec();
    }
    inline void exit(int code) { blocker->exit(code); }
    inline void setTimeout(int msec) { timer->setInterval(msec); }

private:
    QVariant ret;
    QEventLoop *blocker { nullptr };
    QScopedPointer<QTimer> timer { nullptr };
};

class DProtocolDevicePrivate final : public DDevicePrivate
{
    friend class DProtocolDevice;

public:
    DProtocolDevicePrivate(const QString &id, GVolumeMonitor *monitor, DProtocolDevice *qq);
    ~DProtocolDevicePrivate();

    QString path() const;
    QString mount(const QVariantMap &opts);
    void mountAsync(const QVariantMap &opts, DeviceOperateCallbackWithMessage cb);
    bool unmount(const QVariantMap &opts);
    void unmountAsync(const QVariantMap &opts, DeviceOperateCallback cb);
    bool rename(const QString &newName);
    void renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCallback cb);
    QString mountPoint() const;
    QString fileSystem() const;
    long sizeTotal() const;
    long sizeUsage() const;
    long sizeFree() const;
    DeviceType deviceType() const;
    QString displayName() const;

    bool eject();
    bool powerOff();

    static QString mountPoint(GMount *mnt);

    enum FsAttr {
        Total,
        Usage,
        Free,
        Type
    };
    QVariant getAttr(FsAttr type) const;

private:
    static void mountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker);
    static void mountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy);

    static void unmountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker);
    static void unmountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy);

private:
    QString deviceId;
    QStringList deviceIcons;

    mutable QMutex mutexForMount;
    GMount *mountHandler { nullptr };
    GVolume *volumeHandler { nullptr };
    GVolumeMonitor *volumeMonitor { nullptr };

    mutable QVariantMap fsAttrs;

    int timeout { 25000 };
};
DFM_MOUNT_END_NS

#endif   // DPROTOCOLDEVICE_P_H
