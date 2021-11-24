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

#include "dfmprotocoldevice.h"
#include "private/dfmprotocoldevice_p.h"
#include "base/dfmmountdefines.h"
#include "base/dfmmountutils.h"

#include <QEventLoop>
#include <QDebug>
#include <QTimer>

#include <functional>

DFM_MOUNT_USE_NS

DFMProtocolDevice::DFMProtocolDevice(const QString &id, GVolume *vol, GMount *mnt, QObject *parent)
    : DFMDevice(new DFMProtocolDevicePrivate(id, vol, mnt, this), parent)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    using namespace std;
    using namespace std::placeholders;
    registerPath(bind(&DFMProtocolDevicePrivate::path, dp));
    registerMount(bind(&DFMProtocolDevicePrivate::mount, dp, _1));
    registerMountAsync(bind(&DFMProtocolDevicePrivate::mountAsync, dp, _1, _2));
    registerUnmount(bind(&DFMProtocolDevicePrivate::unmount, dp, _1));
    registerUnmountAsync(bind(&DFMProtocolDevicePrivate::unmountAsync, dp, _1, _2));
    registerRename(bind(&DFMProtocolDevicePrivate::rename, dp, _1));
    registerRenameAsync(bind(&DFMProtocolDevicePrivate::renameAsync, dp, _1, _2, _3));
    auto mountPointOfClass = static_cast<QString(DFMProtocolDevicePrivate::*)()const>(&DFMProtocolDevicePrivate::mountPoint);
    registerMountPoint(bind(mountPointOfClass, dp));
    registerFileSystem(bind(&DFMProtocolDevicePrivate::fileSystem, dp));
    registerSizeTotal(bind(&DFMProtocolDevicePrivate::sizeTotal, dp));
    registerSizeUsage(bind(&DFMProtocolDevicePrivate::sizeUsage, dp));
    registerSizeFree(bind(&DFMProtocolDevicePrivate::sizeFree, dp));
    registerDeviceType(bind(&DFMProtocolDevicePrivate::deviceType, dp));
    registerDisplayName(bind(&DFMProtocolDevicePrivate::displayName, dp));
}

void DFMProtocolDevice::setVolume(GVolume *vol)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
    if (!dp)
        return;
    dp->setVolume(vol);
}

void DFMProtocolDevice::setMount(GMount *mnt)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
    if (!dp)
        return;
    dp->setMount(mnt);
}

DFMProtocolDevice::~DFMProtocolDevice()
{
    qDebug() << __FUNCTION__ << "is released...";
}

DFMProtocolDevicePrivate::DFMProtocolDevicePrivate(const QString &id, GVolume *vol, GMount *mnt, DFMProtocolDevice *qq)
    : DFMDevicePrivate(qq), deviceId(id), mountHandler(mnt), volumeHandler(vol)
{

}

QString DFMProtocolDevicePrivate::path() const
{
    return deviceId;
}

QString DFMProtocolDevicePrivate::mount(const QVariantMap &opts)
{
    if (!opts.contains(ParamCancellable))
        qWarning() << "Cancellable is not defined, may cause problem";

    if (mountHandler) {
        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker locker(&mutexForMount);
        qInfo() << "mutexForMount locked" << __FUNCTION__;
        lastError = DeviceError::AlreadyMounted;
        return mountPoint(mountHandler);
    }

    if (volumeHandler) {
        GCancellable *cancellable { nullptr };
        GMountOperation *operation { nullptr };
        if (opts.contains(ParamCancellable))
            cancellable = reinterpret_cast<GCancellable *>((opts.value(ParamCancellable).value<void *>()));
        if (opts.contains(ParamMountOperation))
            operation = reinterpret_cast<GMountOperation *>((opts.value(ParamMountOperation).value<void *>()));

        qInfo() << "mutexForVolume prelocked" << __FUNCTION__;
        QMutexLocker locker(&mutexForVolume);
        qInfo() << "mutexForVolume locked" << __FUNCTION__;
        QScopedPointer<ASyncToSyncHelper> blocker(new ASyncToSyncHelper(this));
        g_volume_mount(volumeHandler, G_MOUNT_MOUNT_NONE, operation, cancellable, mountAsyncCallback, blocker.data());
        auto ret = blocker->exec();
        if (ret == ASyncToSyncHelper::NoError) {
            auto mpt = blocker->result().toString();
            return mpt;
        } else if (ret == ASyncToSyncHelper::Timeout) {
            if (cancellable)
                g_cancellable_cancel(cancellable);
            lastError = DeviceError::TimedOut;
        }
    }
    lastError = DeviceError::NotMountable;
    return "";
}

void DFMProtocolDevicePrivate::mountAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
// TODO
}

bool DFMProtocolDevicePrivate::unmount(const QVariantMap &opts)
{
    GMountUnmountFlags flag;
    if (opts.contains(ParamForce) && opts.value(ParamForce).toBool())
        flag = GMountUnmountFlags::G_MOUNT_UNMOUNT_FORCE;
    else
        flag = GMountUnmountFlags::G_MOUNT_UNMOUNT_NONE;

    if (mountHandler) {
        GCancellable *cancellable { nullptr };
        GMountOperation *operation { nullptr };
        if (opts.contains(ParamCancellable))
            cancellable = reinterpret_cast<GCancellable *>((opts.value(ParamCancellable).value<void *>()));
        if (opts.contains(ParamMountOperation))
            operation = reinterpret_cast<GMountOperation *>((opts.value(ParamMountOperation).value<void *>()));

        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker locker(&mutexForMount);
        qInfo() << "mutexForMount locked" << __FUNCTION__;
        QScopedPointer<ASyncToSyncHelper> blocker(new ASyncToSyncHelper(this));
        g_mount_unmount_with_operation(mountHandler, flag, operation, cancellable, unmountAsyncCallback, blocker.data());
        auto ret = blocker->exec();
        if (ret == ASyncToSyncHelper::NoError) {
            return true;
        } else if (ret == ASyncToSyncHelper::Timeout) {
            lastError = DeviceError::TimedOut;
            return false;
        }
    }
    lastError = DeviceError::NotMounted;
    return false;
}

void DFMProtocolDevicePrivate::unmountAsync(const QVariantMap &opts, DeviceOperateCb cb)
{
// TODO
}

bool DFMProtocolDevicePrivate::rename(const QString &newName)
{// TODO
    return false;
}

void DFMProtocolDevicePrivate::renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCb cb)
{
// TODO
}

QString DFMProtocolDevicePrivate::mountPoint() const
{
    if (mountHandler)
        return mountPoint(mountHandler);
    return QString();
}

QString DFMProtocolDevicePrivate::fileSystem() const
{
    // TODO
    return QString();
}

long DFMProtocolDevicePrivate::sizeTotal() const
{
    // TODO
    return 0;
}

long DFMProtocolDevicePrivate::sizeUsage() const
{
    // TODO
    return 0;
}

long DFMProtocolDevicePrivate::sizeFree() const
{
    // TODO
    return 0;
}

DeviceType DFMProtocolDevicePrivate::deviceType() const
{
    // TODO
    return DeviceType::ProtocolDevice;
}

QString DFMProtocolDevicePrivate::displayName() const
{
    if (volumeHandler) {
        qInfo() << "mutexForVolume prelock" << __FUNCTION__;
        QMutexLocker volLocker(&mutexForVolume);
        qInfo() << "mutexForVolume locked" << __FUNCTION__;
        char *displayName = g_volume_get_name(volumeHandler);
        QString name(displayName);
        g_free(displayName);
        return name;
    }

    if (mountHandler) {
        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker mntLocker(&mutexForMount);
        qInfo() << "mutexForMount locked" << __FUNCTION__;
        char *displayName = g_mount_get_name(mountHandler);
        QString name(displayName);
        g_free(displayName);
        return name;
    }

    lastError = DeviceError::NotMountable;
    return "";
}

bool DFMProtocolDevicePrivate::eject()
{
    // TODO
    return false;
}

bool DFMProtocolDevicePrivate::powerOff()
{
    // TODO
    return false;
}

QString DFMProtocolDevicePrivate::mountPoint(GMount *mount)
{
    QString mpt;
    auto mntRoot = g_mount_get_root(mount);
    if (mntRoot) {
        char *mntPath = g_file_get_path(mntRoot);
        mpt = QString(mntPath);
        g_free(mntPath);
        g_object_unref(mntRoot);
    }
    return mpt;
}

void DFMProtocolDevicePrivate::mountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    auto helper = static_cast<ASyncToSyncHelper *>(user_data);
    auto vol = reinterpret_cast<GVolume *>(source_object);

    QString mpt;
    GError *err { nullptr };
    bool ret = g_volume_mount_finish(vol, res, &err);
    auto mount = g_volume_get_mount(vol);
    if (mount) {
        mpt = mountPoint(mount);
        g_object_unref(mount);
    }
    if (err) {
        // TODO: handle error
        g_error_free(err);
    }

    if (helper) {
        auto code = ret ? ASyncToSyncHelper::NoError : ASyncToSyncHelper::Failed;
        helper->caller()->setMount(mount);
        helper->setResult(mpt);
        helper->exit(code);
    }
}

void DFMProtocolDevicePrivate::unmountAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    auto helper = static_cast<ASyncToSyncHelper *>(user_data);
    auto mnt = reinterpret_cast<GMount *>(source_object);

    GError *err { nullptr };
    bool ret = g_mount_unmount_with_operation_finish(mnt, res, &err);
    if (err) {
        // TODO
        g_error_free(err);
    }

    if (helper) {
        auto code = ret ? ASyncToSyncHelper::NoError : ASyncToSyncHelper::Failed;
        helper->setResult(ret);
        helper->exit(code);
    }
}

ASyncToSyncHelper::ASyncToSyncHelper(DFMProtocolDevicePrivate *dev)
    : deviced(dev) {
    blocker = new QEventLoop();

    timer.reset(new QTimer());
    timer->setInterval(25000);
    timer->setSingleShot(true);
    QObject::connect(timer.data(), &QTimer::timeout, blocker, [this]{
        blocker->exit(Timeout);
    });
}

ASyncToSyncHelper::~ASyncToSyncHelper() {
    if (blocker) {
        blocker->exit();
        delete blocker;
        blocker = nullptr;
    }
    timer->stop();
}
