/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     xushitong<xushitong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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
#ifndef DFMPROTOCOLDEVICE_P_H
#define DFMPROTOCOLDEVICE_P_H

#include "base/dfmmountdefines.h"
#include "private/dfmdevice_p.h"
#include "dfmprotocoldevice.h"

#include <QMutex>
#include <QMutexLocker>
#include <QEventLoop>
#include <QTimer>

#include <gio/gio.h>

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

class DFMProtocolDevicePrivate final : public DFMDevicePrivate
{
    friend class DFMProtocolDevice;

public:
    DFMProtocolDevicePrivate(const QString &id, GVolumeMonitor *monitor, DFMProtocolDevice *qq);
    ~DFMProtocolDevicePrivate();

    QString path() const;
    QString mount(const QVariantMap &opts);
    void mountAsync(const QVariantMap &opts, DeviceOperateCb cb);
    bool unmount(const QVariantMap &opts);
    void unmountAsync(const QVariantMap &opts, DeviceOperateCb cb);
    bool rename(const QString &newName);
    void renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCb cb);
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

public:
    QString deviceId;

private:
    static void mountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker);
    static void mountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy);

    static void unmountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker);
    static void unmountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy);

private:
    mutable QMutex mutexForMount;
    GMount *mountHandler { nullptr };
    GVolume *volumeHandler { nullptr };
    GVolumeMonitor *volumeMonitor { nullptr };

    int timeout { 25000 };
};
DFM_MOUNT_END_NS

#endif   // DFMPROTOCOLDEVICE_P_H
