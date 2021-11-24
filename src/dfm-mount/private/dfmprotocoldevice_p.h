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
class ASyncToSyncHelper {
public:
    enum {
        NoError,
        Failed,
        Timeout,
    };
    explicit ASyncToSyncHelper(DFMProtocolDevicePrivate *dev);
    ~ASyncToSyncHelper();
    ASyncToSyncHelper(const ASyncToSyncHelper &) = delete;
    ASyncToSyncHelper& operator=(const ASyncToSyncHelper &) = delete;

    inline QVariant     result()                    { return ret; }
    inline void         setResult(QVariant result)  { ret = result; }
    inline int          exec()                      {
        timer->start();
        return blocker->exec();
    }
    inline void         exit(int code)              { blocker->exit(code); }
    inline void         setTimeout(int msec)        { timer->setInterval(msec); }
    inline DFMProtocolDevicePrivate *caller()       { return deviced; }

private:
    QVariant                    ret;
    QEventLoop                  *blocker    { nullptr };
    DFMProtocolDevicePrivate    *deviced    { nullptr };
    QScopedPointer<QTimer>      timer       { nullptr };
};
class DFMProtocolDevicePrivate final : public DFMDevicePrivate
{
public:
    DFMProtocolDevicePrivate(const QString &id, GVolume *vol, GMount *mnt, DFMProtocolDevice *qq);

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

    inline void setMount(GMount *mount) {
        QMutexLocker locker(&mutexForMount);
        mountHandler = mount;
    }
    inline void setVolume(GVolume *volume) {
        QMutexLocker locker(&mutexForVolume);
        volumeHandler = volume;
    }

public:
    QString deviceId; // device id, which is a generated uuid

private:
    static void mountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);
    static void unmountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data);

private:
    mutable QMutex  mutexForMount;
    mutable QMutex  mutexForVolume;
    GMount          *mountHandler     { nullptr };
    GVolume         *volumeHandler    { nullptr };
};
DFM_MOUNT_END_NS

#endif // DFMPROTOCOLDEVICE_P_H
