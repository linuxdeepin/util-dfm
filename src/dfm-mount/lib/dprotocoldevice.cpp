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

#include "base/dmount_global.h"
#include "base/dmountutils.h"
#include "dprotocoldevice.h"
#include "private/dprotocoldevice_p.h"
#include "private/dnetworkmounter.h"

#include <QEventLoop>
#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QElapsedTimer>
#include <QRegularExpression>

#include <functional>

DFM_MOUNT_USE_NS

namespace {
struct CallbackProxyWithData
{
    CallbackProxyWithData() = delete;
    explicit CallbackProxyWithData(DeviceOperateCallback cb)
        : caller(cb) {}
    explicit CallbackProxyWithData(DeviceOperateCallbackWithMessage cb)
        : caller(cb) {}
    CallbackProxy caller;
    QPointer<DProtocolDevice> data;
    DProtocolDevicePrivate *d { nullptr };
};
}

DProtocolDevice::DProtocolDevice(const QString &id, GVolumeMonitor *monitor, QObject *parent)
    : DDevice(new DProtocolDevicePrivate(id, monitor, this), parent)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DProtocolDevicePrivate>(d.data());
    if (!dp) {
        qCritical() << "private pointer not valid" << __PRETTY_FUNCTION__;
        abort();
    }
    using namespace std;
    using namespace std::placeholders;
    registerPath(bind(&DProtocolDevicePrivate::path, dp));
    registerMount(bind(&DProtocolDevicePrivate::mount, dp, _1));
    registerMountAsync(bind(&DProtocolDevicePrivate::mountAsync, dp, _1, _2));
    registerUnmount(bind(&DProtocolDevicePrivate::unmount, dp, _1));
    registerUnmountAsync(bind(&DProtocolDevicePrivate::unmountAsync, dp, _1, _2));
    registerRename(bind(&DProtocolDevicePrivate::rename, dp, _1));
    registerRenameAsync(bind(&DProtocolDevicePrivate::renameAsync, dp, _1, _2, _3));
    registerFileSystem(bind(&DProtocolDevicePrivate::fileSystem, dp));
    registerSizeTotal(bind(&DProtocolDevicePrivate::sizeTotal, dp));
    registerSizeUsage(bind(&DProtocolDevicePrivate::sizeUsage, dp));
    registerSizeFree(bind(&DProtocolDevicePrivate::sizeFree, dp));
    registerDeviceType(bind(&DProtocolDevicePrivate::deviceType, dp));
    registerDisplayName(bind(&DProtocolDevicePrivate::displayName, dp));
    auto mountPointOfClass = static_cast<QString (DProtocolDevicePrivate::*)() const>(&DProtocolDevicePrivate::mountPoint);
    registerMountPoint(bind(mountPointOfClass, dp));
}

void DProtocolDevice::mounted(const QString &id)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DProtocolDevicePrivate>(d.data());
    if (!dp || id != dp->deviceId)
        return;

    if (dp->mountHandler) {   // release the old one, keep the handler is the newest.
        QMutexLocker locker(&dp->mutexForMount);
        g_object_unref(dp->mountHandler);
        dp->mountHandler = nullptr;
    }

    auto mnts = g_volume_monitor_get_mounts(dp->volumeMonitor);
    while (mnts) {
        auto mnt = reinterpret_cast<GMount *>(mnts->data);
        g_autoptr(GFile) mntRoot = g_mount_get_root(mnt);
        if (mntRoot) {
            g_autofree char *curi = g_file_get_uri(mntRoot);
            if (QString(curi) == id) {
                QMutexLocker locker(&dp->mutexForMount);
                dp->mountHandler = reinterpret_cast<GMount *>(g_object_ref(mnt));
                break;
            }
        }
        mnts = mnts->next;
    }
    g_list_free_full(mnts, g_object_unref);
}

void DProtocolDevice::unmounted(const QString &id)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DProtocolDevicePrivate>(d.data());
    if (!dp || id != dp->deviceId)
        return;

    QMutexLocker locker(&dp->mutexForMount);
    g_object_unref(dp->mountHandler);
    dp->mountHandler = nullptr;
}

DProtocolDevice::~DProtocolDevice()
{
}

QStringList DProtocolDevice::deviceIcons() const
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DProtocolDevicePrivate>(d.data());
    if (!dp)
        return {};
    if (!dp->deviceIcons.isEmpty())
        return dp->deviceIcons;

    GIcon_autoptr icon { nullptr };
    if (dp->volumeHandler) {
        icon = g_volume_get_icon(dp->volumeHandler);
    } else if (dp->mountHandler) {
        icon = g_mount_get_icon(dp->mountHandler);
    } else {
        return {};
    }

    if (icon) {
        g_autofree char *cname = g_icon_to_string(icon);
        if (cname) {
            // iconName: . GThemedIcon drive-removable-media drive-removable drive drive-removable-media-symbolic drive-removable-symbolic drive-symbolic
            QString iconNames(cname);
            iconNames.remove(". GThemedIcon");
            auto iconLst = iconNames.split(" ", QString::SkipEmptyParts);
            dp->deviceIcons = iconLst;
            return iconLst;
        }
    }
    return {};
}

/*!
 * \brief DProtocolDevice::mountNetworkDevice
 * \param address       remote link address like 'smb://1.2.3.4/sharefolder/
 * \param getPassInfo   an passwd-asking dialog should be exec in this function, and return a struct object which contains the passwd
 * \param mountResult   when mount finished, this function will be invoked.
 */
void DProtocolDevice::mountNetworkDevice(const QString &address, GetMountPassInfo getPassInfo, GetUserChoice getUserChoice, DeviceOperateCallbackWithMessage mountResult, int msecs)
{
    DNetworkMounter::mountNetworkDev(address, getPassInfo, getUserChoice, mountResult, msecs);
}

void DProtocolDevice::setOperatorTimeout(int msecs)
{
    auto dp = Utils::castClassFromTo<DDevicePrivate, DProtocolDevicePrivate>(d.data());
    if (dp)
        dp->timeout = msecs;
}

DProtocolDevicePrivate::DProtocolDevicePrivate(const QString &id, GVolumeMonitor *monitor, DProtocolDevice *qq)
    : DDevicePrivate(qq), deviceId(id), volumeMonitor(monitor)
{
    auto vols = g_volume_monitor_get_volumes(monitor);
    while (vols) {
        auto vol = reinterpret_cast<GVolume *>(vols->data);
        g_autoptr(GFile) volRoot = g_volume_get_activation_root(vol);
        if (volRoot) {
            g_autofree char *curi = g_file_get_uri(volRoot);
            if (QString(curi) == id) {
                this->volumeHandler = reinterpret_cast<GVolume *>(g_object_ref(vol));
                break;
            }
        }
        vols = vols->next;
    }
    g_list_free_full(vols, g_object_unref);

    auto mnts = g_volume_monitor_get_mounts(monitor);
    while (mnts) {
        auto mnt = reinterpret_cast<GMount *>(mnts->data);
        g_autoptr(GFile) mntRoot = g_mount_get_root(mnt);
        if (mntRoot) {
            g_autofree char *curi = g_file_get_uri(mntRoot);
            if (QString(curi) == id) {
                this->mountHandler = reinterpret_cast<GMount *>(g_object_ref(mnt));
                break;
            }
        }
        mnts = mnts->next;
    }
    g_list_free_full(mnts, g_object_unref);
}

DProtocolDevicePrivate::~DProtocolDevicePrivate()
{
    if (mountHandler)
        g_object_unref(mountHandler);
    if (volumeHandler)
        g_object_unref(volumeHandler);
}

QString DProtocolDevicePrivate::path() const
{
    return deviceId;
}

QString DProtocolDevicePrivate::mount(const QVariantMap &opts)
{
    if (mountHandler) {
        //        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker locker(&mutexForMount);
        //        qInfo() << "mutexForMount locked" << __FUNCTION__;
        lastError = DeviceError::kUserErrorAlreadyMounted;
        return mountPoint(mountHandler);
    }

    if (volumeHandler) {
        GMountOperation *operation { nullptr };
        if (opts.contains(ParamMountOperation))
            operation = reinterpret_cast<GMountOperation *>((opts.value(ParamMountOperation).value<void *>()));

        //        qInfo() << "mutexForVolume prelocked" << __FUNCTION__;
        //        QMutexLocker locker(&mutexForVolume);
        //        qInfo() << "mutexForVolume locked" << __FUNCTION__;

        if (!g_volume_can_mount(volumeHandler)) {
            lastError = DeviceError::kUserErrorNotMountable;
            return "";
        }

        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        QScopedPointer<ASyncToSyncHelper> blocker(new ASyncToSyncHelper(timeout));
        g_volume_mount(volumeHandler, G_MOUNT_MOUNT_NONE, operation, cancellable, mountWithBlocker, blocker.data());
        auto ret = blocker->exec();
        if (ret == ASyncToSyncHelper::NoError) {
            auto mpt = blocker->result().toString();
            g_autoptr(GMount) mnt = g_volume_get_mount(volumeHandler);
            //            setMount(mnt);
            return mpt;
        } else if (ret == ASyncToSyncHelper::Timeout) {
            if (cancellable)
                g_cancellable_cancel(cancellable);
            lastError = DeviceError::kUserErrorTimedOut;
        }
    }
    lastError = DeviceError::kUserErrorNotMountable;
    return "";
}

void DProtocolDevicePrivate::mountAsync(const QVariantMap &opts, DeviceOperateCallbackWithMessage cb)
{
    if (mountHandler) {
        //        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker locker(&mutexForMount);
        //        qInfo() << "mutexForMount locked" << __FUNCTION__;
        lastError = DeviceError::kUserErrorAlreadyMounted;
        if (cb)
            cb(true, lastError, mountPoint(mountHandler));
        return;
    }

    if (volumeHandler) {
        if (!g_volume_can_mount(volumeHandler)) {
            lastError = DeviceError::kUserErrorNotMountable;
            if (cb)
                cb(false, lastError, "");
            return;
        }

        if (!opts.contains(ParamCancellable))
            qWarning() << "Cancellable is not defined, may cause problem";

        GCancellable *cancellable { nullptr };
        GMountOperation *operation { nullptr };
        if (opts.contains(ParamCancellable))
            cancellable = reinterpret_cast<GCancellable *>((opts.value(ParamCancellable).value<void *>()));
        if (opts.contains(ParamMountOperation))
            operation = reinterpret_cast<GMountOperation *>((opts.value(ParamMountOperation).value<void *>()));

        CallbackProxyWithData *proxy = new CallbackProxyWithData(cb);
        proxy->data = qobject_cast<DProtocolDevice *>(q);
        proxy->d = this;
        g_volume_mount(volumeHandler, G_MOUNT_MOUNT_NONE, operation, cancellable, mountWithCallback, proxy);
    }
}

bool DProtocolDevicePrivate::unmount(const QVariantMap &opts)
{
    if (!mountHandler) {
        lastError = DeviceError::kUserErrorNotMounted;
        return true;
    } else {
        QString mpt = mountPoint(mountHandler);
        if (mpt.contains(QRegularExpression("^/media/.*/smbmounts/")) && DNetworkMounter::isDaemonMountEnable()) {
            return DNetworkMounter::unmountNetworkDev(mpt);
        } else {
            GMountOperation *operation { nullptr };
            if (opts.contains(ParamMountOperation))
                operation = reinterpret_cast<GMountOperation *>((opts.value(ParamMountOperation).value<void *>()));

            GMountUnmountFlags flag;
            if (opts.contains(ParamForce) && opts.value(ParamForce).toBool())
                flag = GMountUnmountFlags::G_MOUNT_UNMOUNT_FORCE;
            else
                flag = GMountUnmountFlags::G_MOUNT_UNMOUNT_NONE;

            g_autoptr(GCancellable) cancellable = g_cancellable_new();
            QScopedPointer<ASyncToSyncHelper> blocker(new ASyncToSyncHelper(timeout));
            g_mount_unmount_with_operation(mountHandler, flag, operation, cancellable, unmountWithBlocker, blocker.data());
            auto ret = blocker->exec();
            if (ret == ASyncToSyncHelper::NoError) {
                return true;
            } else if (ret == ASyncToSyncHelper::Timeout) {
                lastError = DeviceError::kUserErrorTimedOut;
                g_cancellable_cancel(cancellable);
                return false;
            }
        }
    }
    return false;
}

void DProtocolDevicePrivate::unmountAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (!mountHandler) {
        lastError = DeviceError::kUserErrorNotMounted;
        if (cb)
            cb(true, lastError);
        return;
    } else {
        QString mpt = mountPoint(mountHandler);
        if (mpt.contains(QRegularExpression("^/media/.*/smbmounts/")) && DNetworkMounter::isDaemonMountEnable()) {
            DNetworkMounter::unmountNetworkDevAsync(mpt, cb);
        } else {
            GCancellable *cancellable { nullptr };
            GMountOperation *operation { nullptr };
            if (opts.contains(ParamCancellable))
                cancellable = reinterpret_cast<GCancellable *>((opts.value(ParamCancellable).value<void *>()));
            if (opts.contains(ParamMountOperation))
                operation = reinterpret_cast<GMountOperation *>((opts.value(ParamMountOperation).value<void *>()));

            GMountUnmountFlags flag;
            if (opts.contains(ParamForce) && opts.value(ParamForce).toBool())
                flag = GMountUnmountFlags::G_MOUNT_UNMOUNT_FORCE;
            else
                flag = GMountUnmountFlags::G_MOUNT_UNMOUNT_NONE;

            //        qInfo() << "mutexForMount prelock" << __FUNCTION__;
            //        QMutexLocker locker(&mutexForMount);
            //        qInfo() << "mutexForMount locked" << __FUNCTION__;

            CallbackProxyWithData *proxy = new CallbackProxyWithData(cb);
            proxy->data = qobject_cast<DProtocolDevice *>(q);
            proxy->d = this;
            g_mount_unmount_with_operation(mountHandler, flag, operation, cancellable, unmountWithCallback, proxy);
        }
    }
}

bool DProtocolDevicePrivate::rename(const QString &newName)
{
    Q_UNUSED(newName);
    qWarning() << "not supported operation" << __FUNCTION__;
    return false;
}

void DProtocolDevicePrivate::renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCallback cb)
{
    Q_UNUSED(newName);
    Q_UNUSED(opts);
    Q_UNUSED(cb);
    qWarning() << "not supported operation" << __FUNCTION__;
}

QString DProtocolDevicePrivate::mountPoint() const
{
    //    qInfo() << "mutexForMount prelock" << __FUNCTION__;
    QMutexLocker locker(&mutexForMount);
    //    qInfo() << "mutexForMount locked" << __FUNCTION__;
    if (mountHandler)
        return mountPoint(mountHandler);
    return QString();
}

QString DProtocolDevicePrivate::fileSystem() const
{
    return getAttr(Type).toString();
}

long DProtocolDevicePrivate::sizeTotal() const
{
    return getAttr(Total).value<long>();
}

long DProtocolDevicePrivate::sizeUsage() const
{
    return sizeTotal() - sizeFree();
}

long DProtocolDevicePrivate::sizeFree() const
{
    return getAttr(Free).value<long>();
}

DeviceType DProtocolDevicePrivate::deviceType() const
{
    return DeviceType::kProtocolDevice;
}

QString DProtocolDevicePrivate::displayName() const
{
    if (volumeHandler) {
        g_autofree char *displayName = g_volume_get_name(volumeHandler);
        QString name(displayName);
        return name;
    }

    if (mountHandler) {
        //        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker mntLocker(&mutexForMount);
        //        qInfo() << "mutexForMount locked" << __FUNCTION__;
        g_autofree char *displayName = g_mount_get_name(mountHandler);
        QString name(displayName);
        return name;
    }

    lastError = DeviceError::kUserErrorNotMountable;
    return "";
}

bool DProtocolDevicePrivate::eject()
{
    qWarning() << "not supported operation" << __FUNCTION__;
    return false;
}

bool DProtocolDevicePrivate::powerOff()
{
    qWarning() << "not supported operation" << __FUNCTION__;
    return false;
}

QString DProtocolDevicePrivate::mountPoint(GMount *mount)
{
    QString mpt;
    g_autoptr(GFile) mntRoot = g_mount_get_root(mount);
    if (mntRoot) {
        g_autofree char *mntPath = g_file_get_path(mntRoot);
        mpt = QString(mntPath);
    }
    return mpt;
}

QVariant DProtocolDevicePrivate::getAttr(DProtocolDevicePrivate::FsAttr type) const
{
    if (fsAttrs.contains(QString::number(type)))
        return fsAttrs.value(QString::number(type));

    if (mountPoint().isEmpty() || deviceId.startsWith("afc://"))   // gvfs cannot handle the afc files.
        return QVariant();

    g_autoptr(GFile) mntFile = g_file_new_for_uri(deviceId.toStdString().data());
    if (!mntFile) {
        lastError = DeviceError::kUnhandledError;
        qDebug() << "cannot create file handler for " << deviceId;
        return QVariant();
    }

    GError *err = nullptr;
    QElapsedTimer t;
    t.start();
    do {
        g_autoptr(GFileInfo) sysInfo = g_file_query_filesystem_info(mntFile, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE "," G_FILE_ATTRIBUTE_FILESYSTEM_FREE "," G_FILE_ATTRIBUTE_FILESYSTEM_USED "," G_FILE_ATTRIBUTE_FILESYSTEM_TYPE, nullptr, &err);
        if (err) {
            int errCode = err->code;
            QString errMsg = err->message;
            lastError = Utils::castFromGError(err);
            g_error_free(err);
            err = nullptr;

            qDebug() << "query filesystem info failed" << deviceId << errMsg;
            if (errCode == G_IO_ERROR_NOT_MOUNTED || errCode == G_IO_ERROR_PERMISSION_DENIED)
                return QVariant();

            QThread::msleep(50);
            qApp->processEvents();
            continue;
        }

        quint64 total = g_file_info_get_attribute_uint64(sysInfo, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
        quint64 free = g_file_info_get_attribute_uint64(sysInfo, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
        quint64 used = g_file_info_get_attribute_uint64(sysInfo, G_FILE_ATTRIBUTE_FILESYSTEM_USED);
        QString fs = g_file_info_get_attribute_as_string(sysInfo, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE);

        fsAttrs.insert(QString::number(FsAttr::Free), free);
        fsAttrs.insert(QString::number(FsAttr::Total), total);
        fsAttrs.insert(QString::number(FsAttr::Usage), used);
        fsAttrs.insert(QString::number(FsAttr::Type), fs);

        if (type == FsAttr::Free)
            return free;
        if (type == FsAttr::Total)
            return total;
        if (type == FsAttr::Usage)
            return used;
        if (type == FsAttr::Type)
            return fs;

    } while (t.elapsed() < 5000);
    qDebug() << "info not obtained after timeout" << deviceId;
    fsAttrs.insert(QString::number(FsAttr::Free), QVariant());
    fsAttrs.insert(QString::number(FsAttr::Total), QVariant());
    fsAttrs.insert(QString::number(FsAttr::Usage), QVariant());
    fsAttrs.insert(QString::number(FsAttr::Type), QVariant());
    return QVariant();
}

static bool mountDone(GObject *sourceObj, GAsyncResult *res, DeviceError &derr)
{
    auto vol = reinterpret_cast<GVolume *>(sourceObj);

    QString mpt;
    GError *err { nullptr };
    bool ret = g_volume_mount_finish(vol, res, &err);
    if (err) {
        derr = Utils::castFromGError(err);
        qDebug() << "mount failed" << err->message;
        g_error_free(err);
    }

    return ret;
}

void DProtocolDevicePrivate::mountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker)
{
    DeviceError err;
    bool ret = mountDone(sourceObj, res, err);
    auto helper = static_cast<ASyncToSyncHelper *>(blocker);
    if (helper) {
        int code = ret ? ASyncToSyncHelper::NoError : ASyncToSyncHelper::Failed;
        if (ret) {
            auto volume = reinterpret_cast<GVolume *>(sourceObj);
            if (volume) {
                GMount_autoptr mount = g_volume_get_mount(volume);
                if (mount)
                    helper->setResult(mountPoint(mount));
            }
        }
        helper->exit(code);
    }
}

void DProtocolDevicePrivate::mountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy)
{
    DeviceError err;
    auto &&ret = mountDone(sourceObj, res, err);
    auto proxy = static_cast<CallbackProxyWithData *>(cbProxy);
    if (proxy) {
        auto volume = reinterpret_cast<GVolume *>(sourceObj);
        if (volume) {
            GMount_autoptr mount = g_volume_get_mount(volume);

            if (proxy->caller.cbWithInfo)
                proxy->caller.cbWithInfo(ret, err, mountPoint(mount));
        }

        delete proxy;
    }
}

void DProtocolDevicePrivate::unmountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker)
{
    auto mnt = reinterpret_cast<GMount *>(sourceObj);

    GError *err { nullptr };
    bool ret = g_mount_unmount_with_operation_finish(mnt, res, &err);
    if (err) {
        // TODO
        qDebug() << err->message;
        g_error_free(err);
    }

    auto helper = static_cast<ASyncToSyncHelper *>(blocker);
    if (helper) {
        auto code = ret ? ASyncToSyncHelper::NoError : ASyncToSyncHelper::Failed;
        helper->setResult(ret);
        helper->exit(code);
    }
}

void DProtocolDevicePrivate::unmountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy)
{
    auto mnt = reinterpret_cast<GMount *>(sourceObj);

    GError *err { nullptr };
    DeviceError derr = DeviceError::kNoError;
    bool ret = g_mount_unmount_with_operation_finish(mnt, res, &err);
    if (err) {
        // TODO
        g_error_free(err);
    }

    auto proxy = static_cast<CallbackProxyWithData *>(cbProxy);
    if (proxy) {
        if (proxy->data) {
            proxy->d->mountHandler = nullptr;
        }
        if (proxy->caller.cb) {
            proxy->caller.cb(ret, derr);
        }
        delete proxy;
    }
}

ASyncToSyncHelper::ASyncToSyncHelper(int timeout)
{
    blocker = new QEventLoop();

    timer.reset(new QTimer());
    timer->setInterval(timeout);
    timer->setSingleShot(true);
    QObject::connect(timer.data(), &QTimer::timeout, blocker, [this] {
        blocker->exit(Timeout);
    });
}

ASyncToSyncHelper::~ASyncToSyncHelper()
{
    if (blocker) {
        blocker->exit();
        delete blocker;
        blocker = nullptr;
    }
    timer->stop();
}
