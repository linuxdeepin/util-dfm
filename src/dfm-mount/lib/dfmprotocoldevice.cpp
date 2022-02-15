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
#include "base/dfmmount_global.h"
#include "base/dfmmountutils.h"

#include <QEventLoop>
#include <QDebug>
#include <QTimer>
#include <QPointer>

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
    QPointer<DFMProtocolDevice> data;
    DFMProtocolDevicePrivate *d { nullptr };
};
}

DFMProtocolDevice::DFMProtocolDevice(const QString &id, GVolumeMonitor *monitor, QObject *parent)
    : DFMDevice(new DFMProtocolDevicePrivate(id, monitor, this), parent)
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
    registerFileSystem(bind(&DFMProtocolDevicePrivate::fileSystem, dp));
    registerSizeTotal(bind(&DFMProtocolDevicePrivate::sizeTotal, dp));
    registerSizeUsage(bind(&DFMProtocolDevicePrivate::sizeUsage, dp));
    registerSizeFree(bind(&DFMProtocolDevicePrivate::sizeFree, dp));
    registerDeviceType(bind(&DFMProtocolDevicePrivate::deviceType, dp));
    registerDisplayName(bind(&DFMProtocolDevicePrivate::displayName, dp));
    auto mountPointOfClass = static_cast<QString (DFMProtocolDevicePrivate::*)() const>(&DFMProtocolDevicePrivate::mountPoint);
    registerMountPoint(bind(mountPointOfClass, dp));
}

void DFMProtocolDevice::mounted(const QString &id)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
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

void DFMProtocolDevice::unmounted(const QString &id)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
    if (!dp || id != dp->deviceId)
        return;

    QMutexLocker locker(&dp->mutexForMount);
    g_object_unref(dp->mountHandler);
    dp->mountHandler = nullptr;
}

DFMProtocolDevice::~DFMProtocolDevice()
{
}

QStringList DFMProtocolDevice::deviceIcons() const
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
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
 * \brief DFMProtocolDevice::mountNetworkDevice
 * \param address       remote link address like 'smb://1.2.3.4/sharefolder/
 * \param getPassInfo   an passwd-asking dialog should be exec in this function, and return a struct object which contains the passwd
 * \param mountResult   when mount finished, this function will be invoked.
 */
void DFMProtocolDevice::mountNetworkDevice(const QString &address, GetMountPassInfo getPassInfo, DeviceOperateCallbackWithMessage mountResult)
{
    GFile_autoptr file = g_file_new_for_uri(address.toStdString().c_str());
    if (!file) {
        qWarning() << "protocol: cannot generate location for" << address;
        return;
    }

    // TODO(xust) after mounted, unmount by dfm, then call this function again, the mntPath could be obtain still,
    // may be a bug in gio.
    //    g_autofree char *mntPath = g_file_get_path(file);
    //    if (mntPath) {
    //        qDebug() << "protocol: network device has been mounted already: " << address << mntPath;
    //        if (mountResult) {
    //            mountResult(true, DeviceError::UserErrorAlreadyMounted);
    //        }
    //        return;
    //    }

    AskPasswdHelper *helper = new AskPasswdHelper();
    helper->callback = getPassInfo;
    helper->callOnceFlag = false;   // make sure the signal will not emit continuously when validate failed.

    GMountOperation_autoptr op = g_mount_operation_new();
    g_signal_connect(op, "ask_password", G_CALLBACK(DFMProtocolDevicePrivate::mountNetworkDeviceAskPasswd), helper);

    FinalizeHelper *finalizeHelper = new FinalizeHelper;
    finalizeHelper->askPasswd = helper;
    finalizeHelper->callback = mountResult;

    // a timout machinism TODO(xust)
    GCancellable_autoptr cancellable = g_cancellable_new();
    g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, cancellable,
                                  &DFMProtocolDevicePrivate::mountNetworkDeviceCallback, finalizeHelper);
}

void DFMProtocolDevice::setOperatorTimeout(int msecs)
{
    auto dp = Utils::castClassFromTo<DFMDevicePrivate, DFMProtocolDevicePrivate>(d.data());
    if (dp)
        dp->timeout = msecs;
}

DFMProtocolDevicePrivate::DFMProtocolDevicePrivate(const QString &id, GVolumeMonitor *monitor, DFMProtocolDevice *qq)
    : DFMDevicePrivate(qq), deviceId(id), volumeMonitor(monitor)
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

DFMProtocolDevicePrivate::~DFMProtocolDevicePrivate()
{
    if (mountHandler)
        g_object_unref(mountHandler);
    if (volumeHandler)
        g_object_unref(volumeHandler);
}

QString DFMProtocolDevicePrivate::path() const
{
    return deviceId;
}

QString DFMProtocolDevicePrivate::mount(const QVariantMap &opts)
{
    if (mountHandler) {
        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker locker(&mutexForMount);
        qInfo() << "mutexForMount locked" << __FUNCTION__;
        lastError = DeviceError::UserErrorAlreadyMounted;
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
            lastError = DeviceError::UserErrorNotMountable;
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
            lastError = DeviceError::UserErrorTimedOut;
        }
    }
    lastError = DeviceError::UserErrorNotMountable;
    return "";
}

void DFMProtocolDevicePrivate::mountAsync(const QVariantMap &opts, DeviceOperateCallbackWithMessage cb)
{
    if (mountHandler) {
        qInfo() << "mutexForMount prelock" << __FUNCTION__;
        QMutexLocker locker(&mutexForMount);
        qInfo() << "mutexForMount locked" << __FUNCTION__;
        lastError = DeviceError::UserErrorAlreadyMounted;
        if (cb)
            cb(true, lastError, mountPoint(mountHandler));
        return;
    }

    if (volumeHandler) {
        if (!g_volume_can_mount(volumeHandler)) {
            lastError = DeviceError::UserErrorNotMountable;
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
        proxy->data = qobject_cast<DFMProtocolDevice *>(q);
        proxy->d = this;
        g_volume_mount(volumeHandler, G_MOUNT_MOUNT_NONE, operation, cancellable, mountWithCallback, proxy);
    }
}

bool DFMProtocolDevicePrivate::unmount(const QVariantMap &opts)
{
    if (!mountHandler) {
        lastError = DeviceError::UserErrorNotMounted;
        return true;
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
            lastError = DeviceError::UserErrorTimedOut;
            g_cancellable_cancel(cancellable);
            return false;
        }
    }
    return false;
}

void DFMProtocolDevicePrivate::unmountAsync(const QVariantMap &opts, DeviceOperateCallback cb)
{
    if (!mountHandler) {
        lastError = DeviceError::UserErrorNotMounted;
        cb(true, lastError);
        return;
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
        proxy->data = qobject_cast<DFMProtocolDevice *>(q);
        proxy->d = this;
        g_mount_unmount_with_operation(mountHandler, flag, operation, cancellable, unmountWithCallback, proxy);
    }
}

bool DFMProtocolDevicePrivate::rename(const QString &newName)
{
    Q_UNUSED(newName);
    qWarning() << "not supported operation" << __FUNCTION__;
    return false;
}

void DFMProtocolDevicePrivate::renameAsync(const QString &newName, const QVariantMap &opts, DeviceOperateCallback cb)
{
    Q_UNUSED(newName);
    Q_UNUSED(opts);
    Q_UNUSED(cb);
    qWarning() << "not supported operation" << __FUNCTION__;
}

QString DFMProtocolDevicePrivate::mountPoint() const
{
    qInfo() << "mutexForMount prelock" << __FUNCTION__;
    QMutexLocker locker(&mutexForMount);
    qInfo() << "mutexForMount locked" << __FUNCTION__;
    if (mountHandler)
        return mountPoint(mountHandler);
    return QString();
}

QString DFMProtocolDevicePrivate::fileSystem() const
{
    return getAttr(Type).toString();
}

long DFMProtocolDevicePrivate::sizeTotal() const
{
    return getAttr(Total).value<long>();
}

long DFMProtocolDevicePrivate::sizeUsage() const
{
    return sizeTotal() - sizeFree();
}

long DFMProtocolDevicePrivate::sizeFree() const
{
    return getAttr(Free).value<long>();
}

DeviceType DFMProtocolDevicePrivate::deviceType() const
{
    return DeviceType::ProtocolDevice;
}

QString DFMProtocolDevicePrivate::displayName() const
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

    lastError = DeviceError::UserErrorNotMountable;
    return "";
}

bool DFMProtocolDevicePrivate::eject()
{
    qWarning() << "not supported operation" << __FUNCTION__;
    return false;
}

bool DFMProtocolDevicePrivate::powerOff()
{
    qWarning() << "not supported operation" << __FUNCTION__;
    return false;
}

QString DFMProtocolDevicePrivate::mountPoint(GMount *mount)
{
    QString mpt;
    g_autoptr(GFile) mntRoot = g_mount_get_root(mount);
    if (mntRoot) {
        g_autofree char *mntPath = g_file_get_path(mntRoot);
        mpt = QString(mntPath);
    }
    return mpt;
}

QVariant DFMProtocolDevicePrivate::getAttr(DFMProtocolDevicePrivate::FsAttr type) const
{
    const char *attr = nullptr;
    switch (type) {
    case Total:
        attr = G_FILE_ATTRIBUTE_FILESYSTEM_SIZE;
        break;
    case Usage:
        attr = G_FILE_ATTRIBUTE_FILESYSTEM_USED;
        break;
    case Free:
        attr = G_FILE_ATTRIBUTE_FILESYSTEM_FREE;
        break;
    case Type:
        attr = G_FILE_ATTRIBUTE_FILESYSTEM_TYPE;
    }

    if (!mountHandler) {
        lastError = DeviceError::UserErrorNotMounted;
        return QVariant();
    }

    //    qInfo() << "mutexForMount prelock" << __FUNCTION__;
    QMutexLocker mntLocker(&mutexForMount);
    //    qInfo() << "mutexForMount locked" << __FUNCTION__;
    auto mnt = mountPoint(mountHandler);
    mntLocker.unlock();

    g_autoptr(GFile) location = g_mount_get_root(mountHandler);
    QString errMsg;
    if (location) {
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        QSharedPointer<QTimer> timer(new QTimer);
        QObject::connect(timer.data(), &QTimer::timeout, q, [=] {
            lastError = DeviceError::UserErrorTimedOut;
            g_cancellable_cancel(cancellable);
        });
        timer->setSingleShot(true);
        timer->start(timeout);

        // before get attributes, check whether the location exist.
        if (!g_file_query_exists(location, cancellable)) {
            errMsg = "dir not exist";
            qWarning() << "get attribute" << attr << "failed: " << errMsg;
            return QVariant();
        }

        int retry = 50;
        while (retry) {
            GError *err { nullptr };
            g_autoptr(GFileInfo) info = g_file_query_filesystem_info(location, attr, cancellable, &err);
            if (info) {
                QVariant ret;
                if (type < Type) {
                    auto size = g_file_info_get_attribute_uint64(info, attr);
                    ret.setValue<long>(static_cast<long>(size));
                    return ret;
                } else if (type == Type) {
                    auto fs = g_file_info_get_attribute_string(info, attr);
                    ret.setValue<QString>(fs);
                    return ret;
                }
            }
            if (err) {
                lastError = Utils::castFromGError(err);
                errMsg = err->message;
                g_error_free(err);
                if (errMsg.contains("was not provided by any .service files"))
                    break;
                retry -= 1;
                QThread::msleep(50);
            }
        }
    }
    qDebug() << "get attribute" << attr << "failed: " << errMsg;
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

void DFMProtocolDevicePrivate::mountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker)
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

void DFMProtocolDevicePrivate::mountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy)
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

void DFMProtocolDevicePrivate::unmountWithBlocker(GObject *sourceObj, GAsyncResult *res, gpointer blocker)
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

void DFMProtocolDevicePrivate::unmountWithCallback(GObject *sourceObj, GAsyncResult *res, gpointer cbProxy)
{
    auto mnt = reinterpret_cast<GMount *>(sourceObj);

    GError *err { nullptr };
    DeviceError derr = DeviceError::NoError;
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

void DFMProtocolDevicePrivate::mountNetworkDeviceAskPasswd(GMountOperation *self, gchar *message, gchar *userDefault,
                                                           gchar *domainDefault, GAskPasswordFlags flags, gpointer userData)
{
    auto helper = reinterpret_cast<AskPasswdHelper *>(userData);
    if (!helper || !helper->callback) {
        helper->err = DeviceError::UserErrorFailed;
        g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    if (!helper->callOnceFlag) {
        helper->callOnceFlag = true;
    } else {
        if (helper->anonymous)
            helper->err = DeviceError::UserErrorNetworkAnonymousNotAllowed;
        else
            helper->err = DeviceError::UserErrorNetworkWrongPasswd;
        g_mount_operation_reply(self, G_MOUNT_OPERATION_UNHANDLED);
        return;
    }

    auto mountInfo = helper->callback(message, userDefault, domainDefault);
    if (mountInfo.cancelled) {
        g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
        helper->err = DeviceError::UserErrorUserCancelled;
        return;
    }

    if (mountInfo.anonymous) {
        // the flags seem to always be 31(0b11111)
        if (!(flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)) {
            helper->err = DeviceError::UserErrorNetworkAnonymousNotAllowed;
            g_mount_operation_reply(self, G_MOUNT_OPERATION_UNHANDLED);
            return;
        }
        helper->anonymous = true;
        g_mount_operation_set_anonymous(self, true);
    } else {
        if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
            g_mount_operation_set_domain(self, mountInfo.domain.toStdString().c_str());
        if (flags & G_ASK_PASSWORD_NEED_USERNAME)
            g_mount_operation_set_username(self, mountInfo.userName.toStdString().c_str());
        if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
            g_mount_operation_set_password(self, mountInfo.passwd.toStdString().c_str());
        g_mount_operation_set_password_save(self, GPasswordSave(mountInfo.savePasswd));
    }
    g_mount_operation_reply(self, G_MOUNT_OPERATION_HANDLED);
}

void DFMProtocolDevicePrivate::mountNetworkDeviceCallback(GObject *srcObj, GAsyncResult *res, gpointer userData)
{
    auto finalize = reinterpret_cast<FinalizeHelper *>(userData);
    if (!finalize)
        return;

    DeviceError derr = finalize->askPasswd->err;
    auto file = reinterpret_cast<GFile *>(srcObj);
    GError_autoptr err = nullptr;
    bool ok = g_file_mount_enclosing_volume_finish(file, res, &err);
    if (!ok && derr == DeviceError::NoError) {
        derr = Utils::castFromGError(err);
    }

    g_autofree char *mntPath = g_file_get_path(file);
    if (finalize->callback)
        finalize->callback(ok, derr, mntPath);

    delete finalize->askPasswd;
    delete finalize;
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
