// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base/dmountutils.h"
#include "dnetworkmounter.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QtConcurrent>

#include <libmount.h>

DFM_MOUNT_USE_NS

static constexpr char kDaemonService[] { "com.deepin.filemanager.daemon" };
static constexpr char kDaemonPath[] { "/com/deepin/filemanager/daemon" };
static constexpr char kDaemonIntro[] { "org.freedesktop.DBus.Introspectable" };
static constexpr char kDaemonIntroMethod[] { "Introspect" };
static constexpr char kMountControlPath[] { "/com/deepin/filemanager/daemon/MountControl" };
static constexpr char kMountControlIFace[] { "com.deepin.filemanager.daemon.MountControl" };
static constexpr char kMountControlMount[] { "Mount" };
static constexpr char kMountControlUnmount[] { "Unmount" };

static constexpr char kSchemaUser[] { "user" };
static constexpr char kSchemaProtocol[] { "protocol" };
static constexpr char kSchemaDomain[] { "domain" };
static constexpr char kSchemaServer[] { "server" };

static constexpr char kLoginUser[] { "user" };
static constexpr char kLoginDomain[] { "domain" };
static constexpr char kLoginPasswd[] { "passwd" };
static constexpr char kLoginTimeout[] { "timeout" };
static constexpr char kMountFsType[] { "fsType" };

static constexpr char kDaemonMountRetKeyMpt[] { "mountPoint" };
static constexpr char kDaemonMountRetKeyErrno[] { "errno" };
static constexpr char kDaemonMountRetKeyErrMsg[] { "errMsg" };

struct AskPasswdHelper
{
    GetMountPassInfo callback { nullptr };
    bool callOnceFlag { false };
    bool anonymous { false };
    DeviceError err { DeviceError::kNoError };
};

struct AskQuestionHelper
{
    GetUserChoice callback { nullptr };
    DeviceError err { DeviceError::kNoError };
};

struct FinalizeHelper
{
    AskPasswdHelper *askPasswd { nullptr };
    AskQuestionHelper *askQuestion { nullptr };
    DeviceOperateCallbackWithMessage resultCallback;
};

bool DNetworkMounter::isDaemonMountEnable()
{
    auto systemBusIFace = QDBusConnection::systemBus().interface();
    if (!systemBusIFace)
        return false;

    if (!systemBusIFace->isServiceRegistered(kDaemonService))
        return false;

    QDBusInterface daemonIface(kDaemonService, kDaemonPath, kDaemonIntro,
                               QDBusConnection::systemBus());
    QDBusReply<QString> reply = daemonIface.call(kDaemonIntroMethod);
    return reply.value().contains("<node name=\"MountControl\"/>");
}

QList<QVariantMap> DNetworkMounter::loginPasswd(const QString &address)
{
    QUrl u(address);
    QString protocol = u.scheme();
    QString host = u.host();

    GHashTable_autoptr query = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert(query, strdup(kSchemaServer), strdup(host.toStdString().c_str()));
    g_hash_table_insert(query, strdup(kSchemaProtocol), strdup(protocol.toStdString().c_str()));

    QList<QVariantMap> passwds;
    GError_autoptr err { nullptr };
    GList_autoptr items = secret_service_search_sync(nullptr, smbSchema(), query, SECRET_SEARCH_ALL,
                                                     nullptr, &err);
    while (items) {
        auto item = static_cast<SecretItem *>(items->data);
        GHashTable_autoptr itemAttrs = secret_item_get_attributes(item);
        QVariantMap attr;
        g_hash_table_foreach(
                itemAttrs,
                [](gpointer k, gpointer v, gpointer vm) {
                    auto info = static_cast<QVariantMap *>(vm);
                    if (!info)
                        return;
                    info->insert(static_cast<char *>(k), static_cast<char *>(v));
                    qDebug() << "######" << *info;
                },
                &attr);
        passwds.append(attr);
        items = items->next;
    }

    for (auto &passwd : passwds) {
        std::string server = passwd.value(kSchemaServer).toString().toStdString();
        std::string protocol = passwd.value(kSchemaProtocol).toString().toStdString();
        std::string user = passwd.value(kSchemaUser).toString().toStdString();
        std::string domain = passwd.value(kSchemaDomain).toString().toStdString();

        GError_autoptr err { nullptr };
        g_autofree char *pwd = secret_password_lookup_sync(
                smbSchema(), nullptr, &err, kSchemaServer, server.c_str(), kSchemaProtocol,
                protocol.c_str(), kSchemaUser, user.c_str(), kSchemaDomain, domain.c_str(),
                nullptr);
        if (err)
            qDebug() << "query password failed: " << passwd << err->message;
        else
            passwd.insert(kLoginPasswd, QString(pwd));
    }
    return passwds;
}

void DNetworkMounter::savePasswd(const QString &address, const MountPassInfo &info)
{
    QUrl u(address);
    QString protocol = u.scheme();
    QString server = u.host();
    const char *collection = info.savePasswd == NetworkMountPasswdSaveMode::kSaveBeforeLogout
            ? SECRET_COLLECTION_SESSION
            : SECRET_COLLECTION_DEFAULT;

    if (protocol == "smb") {
        GError_autoptr err { nullptr };
        QString title = QString("%1@%2")
                                .arg(info.userName)
                                .arg(server);   // username@host, just like the way gvfs do.
        secret_password_store_sync(smbSchema(), collection, title.toStdString().c_str(),
                                   info.passwd.toStdString().c_str(), nullptr, &err, kSchemaDomain,
                                   info.domain.toStdString().c_str(), kSchemaProtocol,
                                   protocol.toStdString().c_str(), kSchemaServer,
                                   server.toStdString().c_str(), kSchemaUser,
                                   info.userName.toStdString().c_str(), nullptr);
        if (err)
            qWarning() << "save passwd failed: " << err->message;
    }
}

SecretSchema *DNetworkMounter::smbSchema()
{
    static SecretSchema sche;
    sche.name = "org.gnome.keyring.NetworkPassword";
    sche.flags = SECRET_SCHEMA_NONE;
    sche.attributes[0] = { kSchemaUser, SECRET_SCHEMA_ATTRIBUTE_STRING };
    sche.attributes[1] = { kSchemaDomain, SECRET_SCHEMA_ATTRIBUTE_STRING };
    sche.attributes[2] = { kSchemaServer, SECRET_SCHEMA_ATTRIBUTE_STRING };
    sche.attributes[3] = { kSchemaProtocol, SECRET_SCHEMA_ATTRIBUTE_STRING };
    return &sche;
}

void DNetworkMounter::mountNetworkDev(const QString &address, GetMountPassInfo getPassInfo,
                                      GetUserChoice getUserChoice,
                                      DeviceOperateCallbackWithMessage mountResult, int secs)
{
    QUrl u(address);
    // don't mount samba's root by Daemon
    if (u.scheme() == "smb" && !u.path().remove("/").isEmpty() && isDaemonMountEnable())
        mountByDaemon(address, getPassInfo, mountResult, secs);
    else
        mountByGvfs(address, getPassInfo, getUserChoice, mountResult, secs);
}

bool DNetworkMounter::unmountNetworkDev(const QString &mpt)
{
    QDBusInterface mntCtrl(kDaemonService, kMountControlPath, kMountControlIFace,
                           QDBusConnection::systemBus());
    QVariantMap opts { { kMountFsType, "cifs" } };
    QDBusReply<QVariantMap> ret = mntCtrl.call(kMountControlUnmount, mpt, opts);
    auto result = ret.value();
    return result.value("result", false).toBool();
}

void DNetworkMounter::unmountNetworkDevAsync(const QString &mpt, DeviceOperateCallback cb)
{
    QFutureWatcher<bool> *watcher { new QFutureWatcher<bool>() };
    QObject::connect(watcher, &QFutureWatcher<bool>::finished, [cb, watcher] {
        bool ret = watcher->result();
        watcher->deleteLater();
        if (cb)
            cb(ret, ret ? DeviceError::kNoError : DeviceError::kUserError);
    });
    watcher->setFuture(QtConcurrent::run(unmountNetworkDev, mpt));
}

/*!
 * \brief DNetworkMounter::mountByDaemon, mount network device (smb/ftp/webdav) by
 * dde-file-manager-daemon. \param address \param getPassInfo \param mountResult \param msecs
 */
void DNetworkMounter::mountByDaemon(const QString &address, GetMountPassInfo getPassInfo,
                                    DeviceOperateCallbackWithMessage mountResult, int secs)
{
    auto requestLoginInfo = [address, getPassInfo] {
        if (getPassInfo)
            return getPassInfo(QObject::tr("need authorization to access %1").arg(address),
                               Utils::currentUser(), "WORKGROUP");
        return MountPassInfo();
    };
    auto checkThread = [] {
        if (QThread::currentThread() != qApp->thread())
            qWarning() << "invoking callback in non-main-thread!!!";
    };

    QString mpt;
    QString addr(QUrl::fromPercentEncoding(address.toLower().toLocal8Bit()));
    if (isMounted(addr, mpt)) {
        if (mountResult)
            mountResult(false, DeviceError::kGIOErrorAlreadyMounted, mpt);
        return;
    }

    auto logins = loginPasswd(addr);
    MountPassInfo loginInfo;
    if (logins.isEmpty()) {
        loginInfo = requestLoginInfo();
        if (loginInfo.cancelled && mountResult) {
            checkThread();
            mountResult(false, DeviceError::kUserErrorUserCancelled, "");
            return;
        }
    }

    QFutureWatcher<MountRet> *watcher { new QFutureWatcher<MountRet>() };
    QObject::connect(watcher, &QFutureWatcher<MountRet>::finished, [=] {
        auto mntRet = watcher->result();
        watcher->deleteLater();
        if (mntRet.requestLoginInfo) {
            auto loginInfo = requestLoginInfo();
            loginInfo.timeout = secs;
            if (loginInfo.cancelled && mountResult) {
                checkThread();
                mountResult(false, DeviceError::kUserErrorUserCancelled, "");
                return;
            }
            doLastMount(addr, loginInfo, mountResult);
        } else {
            if (mountResult) {
                checkThread();
                mountResult(mntRet.ok, mntRet.err, mntRet.mpt);
            }
        }
    });

    loginInfo.timeout = secs;
    auto fu = QtConcurrent::run([=] {
        if (logins.isEmpty())   // try mount with user's input (loginInfo)
            return mountWithUserInput(addr, loginInfo);
        else
            return mountWithSavedInfos(addr, logins, loginInfo.timeout);
    });
    watcher->setFuture(fu);
}

void DNetworkMounter::mountByGvfs(const QString &address, GetMountPassInfo getPassInfo,
                                  GetUserChoice getUserChoice,
                                  DeviceOperateCallbackWithMessage mountResult, int msecs)
{
    auto newAddr = address;
    if (address.startsWith("ftp") && msecs != 0) {   // only ftp-gvfs supports timeout param now.
        int sec = msecs < 1000 && msecs != 0 ? 1 : msecs / 1000;
        newAddr += QString("?socket_timeout=%1").arg(sec);
    }

    GFile_autoptr file = g_file_new_for_uri(newAddr.toStdString().c_str());
    if (!file) {
        qWarning() << "protocol: cannot generate location for" << newAddr;
        return;
    }

    AskPasswdHelper *passwdHelper = new AskPasswdHelper();
    passwdHelper->callback = getPassInfo;
    passwdHelper->callOnceFlag =
            false;   // make sure the signal will not emit continuously when validate failed.

    AskQuestionHelper *questionHelper = new AskQuestionHelper();
    questionHelper->callback = getUserChoice;

    GMountOperation_autoptr op = g_mount_operation_new();
    g_signal_connect(op, "ask_question", G_CALLBACK(DNetworkMounter::mountByGvfsAskQuestion),
                     questionHelper);
    g_signal_connect(op, "ask_password", G_CALLBACK(DNetworkMounter::mountByGvfsAskPasswd),
                     passwdHelper);

    FinalizeHelper *finalizeHelper = new FinalizeHelper;
    finalizeHelper->askPasswd = passwdHelper;
    finalizeHelper->askQuestion = questionHelper;
    finalizeHelper->resultCallback = mountResult;

    GCancellable_autoptr cancellable = nullptr; /*g_cancellable_new();
     if (msecs > 0) {
         QTimer::singleShot(msecs, [cancellable] {
             if (cancellable)
                 g_cancellable_cancel(cancellable);
         });
     }*/
    g_file_mount_enclosing_volume(file, G_MOUNT_MOUNT_NONE, op, cancellable,
                                  &DNetworkMounter::mountByGvfsCallback, finalizeHelper);
}

void DNetworkMounter::mountByGvfsAskQuestion(GMountOperation *self, const char *message,
                                             const char **choices, gpointer userData)
{
    auto helper = reinterpret_cast<AskQuestionHelper *>(userData);
    if (!helper || !helper->callback) {
        if (helper)
            helper->err = DeviceError::kUserErrorFailed;
        g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    QString sMsg(message);
    QStringList lstChoices;
    while (*choices)
        lstChoices << QString::asprintf("%s", *choices++);

    int choice = helper->callback(sMsg, lstChoices);
    if (choice < 0 || choice >= lstChoices.count()) {
        g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    g_mount_operation_set_choice(self, choice);
    g_mount_operation_reply(self, G_MOUNT_OPERATION_HANDLED);
}

void DNetworkMounter::mountByGvfsAskPasswd(GMountOperation *self, gchar *message,
                                           gchar *defaultUser, gchar *defaultDomain,
                                           GAskPasswordFlags flags, gpointer userData)
{
    auto helper = reinterpret_cast<AskPasswdHelper *>(userData);
    if (!helper || !helper->callback) {
        if (helper)
            helper->err = DeviceError::kUserErrorFailed;
        g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    if (!helper->callOnceFlag) {
        helper->callOnceFlag = true;
    } else {
        if (helper->anonymous)
            helper->err = DeviceError::kUserErrorNetworkAnonymousNotAllowed;
        else
            helper->err = DeviceError::kUserErrorNetworkWrongPasswd;
        g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    auto mountInfo = helper->callback(message, defaultUser, defaultDomain);
    if (mountInfo.cancelled) {
        g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
        helper->err = DeviceError::kUserErrorUserCancelled;
        return;
    }

    if (mountInfo.anonymous) {
        // the flags seem to always be 31(0b11111)
        if (!(flags & G_ASK_PASSWORD_ANONYMOUS_SUPPORTED)) {
            helper->err = DeviceError::kUserErrorNetworkAnonymousNotAllowed;
            g_mount_operation_reply(self, G_MOUNT_OPERATION_ABORTED);
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

void DNetworkMounter::mountByGvfsCallback(GObject *srcObj, GAsyncResult *res, gpointer userData)
{
    auto finalize = reinterpret_cast<FinalizeHelper *>(userData);
    if (!finalize)
        return;

    DeviceError derr = finalize->askPasswd->err;
    auto file = reinterpret_cast<GFile *>(srcObj);
    GError_autoptr err = nullptr;
    bool ok = g_file_mount_enclosing_volume_finish(file, res, &err);
    if (!ok && derr == DeviceError::kNoError)
        derr = Utils::castFromGError(err);

    g_autofree char *mntPath = g_file_get_path(file);
    if (finalize->resultCallback)
        finalize->resultCallback(ok, derr, mntPath);

    delete finalize->askPasswd;
    delete finalize->askQuestion;
    delete finalize;
}

DNetworkMounter::MountRet DNetworkMounter::mountWithUserInput(const QString &address,
                                                              const MountPassInfo info)
{
    QVariantMap param { { kLoginUser, info.userName },
                        { kLoginDomain, info.domain },
                        { kLoginPasswd, info.passwd },
                        { kLoginTimeout, info.timeout },
                        { kMountFsType, "cifs" } };

    QDBusInterface mntCtrl(kDaemonService, kMountControlPath, kMountControlIFace,
                           QDBusConnection::systemBus());

    QDBusReply<QVariantMap> ret = mntCtrl.call(kMountControlMount, address, param);
    auto mntRet = ret.value();
    QString mpt = mntRet.value(kDaemonMountRetKeyMpt).toString();
    int errNum = mntRet.value(kDaemonMountRetKeyErrno).toInt();

    bool ok = !mpt.isEmpty();
    DeviceError err = info.anonymous ? DeviceError::kUserErrorNetworkAnonymousNotAllowed
                                     : static_cast<DeviceError>(errNum);
    if (ok) {
        err = DeviceError::kNoError;

        if (!info.anonymous && info.savePasswd != NetworkMountPasswdSaveMode::kNeverSavePasswd)
            savePasswd(address, info);
    }

    return { ok, err, mpt };
}

DNetworkMounter::MountRet DNetworkMounter::mountWithSavedInfos(const QString &address,
                                                               const QList<QVariantMap> &infos,
                                                               int secs)
{
    QDBusInterface mntCtrl(kDaemonService, kMountControlPath, kMountControlIFace,
                           QDBusConnection::systemBus());

    for (const auto &login : infos) {
        QVariantMap param { { kLoginUser, login.value(kSchemaUser) },
                            { kLoginDomain, login.value(kSchemaDomain) },
                            { kLoginPasswd, login.value(kLoginPasswd) },
                            { kLoginTimeout, secs },
                            { kMountFsType, "cifs" } };

        QDBusReply<QVariantMap> ret = mntCtrl.call(kMountControlMount, address, param);
        auto mntRet = ret.value();
        QString mpt = mntRet.value(kDaemonMountRetKeyMpt).toString();

        if (!mpt.isEmpty())
            return { true, DeviceError::kNoError, mpt };
    }

    // when all saved login data is tried, get info from user
    MountRet ret { .requestLoginInfo = true };
    return ret;
}

void DNetworkMounter::doLastMount(const QString &address, const MountPassInfo info,
                                  DeviceOperateCallbackWithMessage cb)
{
    QFutureWatcher<MountRet> *watcher { new QFutureWatcher<MountRet>() };
    QObject::connect(watcher, &QFutureWatcher<MountRet>::finished, [cb, watcher] {
        auto mntRet = watcher->result();
        watcher->deleteLater();
        if (cb) {
            if (QThread::currentThread() != qApp->thread())
                qWarning() << "invoking callback in non-main-thread!!!";
            cb(mntRet.ok, mntRet.err, mntRet.mpt);
        }
    });
    watcher->setFuture(QtConcurrent::run([=] { return mountWithUserInput(address, info); }));
}

bool DNetworkMounter::isMounted(const QString &address, QString &mpt)
{
    class Helper
    {
    public:
        Helper() { tab = mnt_new_table(); }
        ~Helper() { mnt_free_table(tab); }
        libmnt_table *tab { nullptr };
    };
    Helper d;
    auto tab = d.tab;
    int ret = mnt_table_parse_mtab(tab, nullptr);
    qDebug() << "parse mtab: " << ret;

    QString stdAddr(address);
    stdAddr.remove("smb:");
    std::string aPath = stdAddr.toStdString();
    auto fs = mnt_table_find_source(tab, aPath.c_str(), MNT_ITER_BACKWARD);
    if (!fs)
        fs = mnt_table_find_target(tab, aPath.c_str(), MNT_ITER_BACKWARD);
    qDebug() << "find mount: " << fs << aPath.c_str();

    if (fs) {
        mpt = mnt_fs_get_target(fs);
        qDebug() << "find mounted at: " << mpt << address;
        QRegularExpression reg("^/media/(.*)/smbmounts/");
        QRegularExpressionMatch match = reg.match(mpt);

        if (match.hasMatch()) {
            auto user = match.captured(1);
            qDebug() << "the mounted mount is mounted by " << user << address;
            auto currUser = Utils::currentUser();
            if (currUser == user) {
                return true;
            } else {
                return false;   // if not mounted by current user, treat it as not mounted.
            }
        } else {
            return false;   // if not mounted at preseted mountpoint, treat it as not mounted.
        }
    } else {
        return false;
    }
}
