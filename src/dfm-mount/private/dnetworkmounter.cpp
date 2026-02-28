// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-mount/base/dmountutils.h>

#include "dnetworkmounter.h"

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusUnixFileDescriptor>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QtConcurrent>

#include <libmount.h>

#include <unistd.h>

DFM_MOUNT_USE_NS

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
static constexpr char kDaemonService[] { "org.deepin.Filemanager.MountControl" };
static constexpr char kDaemonPath[] { "/org/deepin/Filemanager" };
static constexpr char kMountControlPath[] { "/org/deepin/Filemanager/MountControl" };
static constexpr char kMountControlIFace[] { "org.deepin.Filemanager.MountControl" };
#else
static constexpr char kDaemonService[] { "com.deepin.filemanager.daemon" };
static constexpr char kDaemonPath[] { "/com/deepin/filemanager/daemon" };
static constexpr char kMountControlPath[] { "/com/deepin/filemanager/daemon/MountControl" };
static constexpr char kMountControlIFace[] { "com.deepin.filemanager.daemon.MountControl" };
#endif

static constexpr char kDaemonIntro[] { "org.freedesktop.DBus.Introspectable" };
static constexpr char kDaemonIntroMethod[] { "Introspect" };
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
static constexpr char kSmbConfigPath[] { "/etc/samba/smb.conf" };

static constexpr char kDaemonMountRetKeyMpt[] { "mountPoint" };
static constexpr char kDaemonMountRetKeyErrno[] { "errno" };

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
    QVariant customData;
};

bool DNetworkMounter::isDaemonMountEnable()
{
    auto systemBusIFace = QDBusConnection::systemBus().interface();
    if (!systemBusIFace)
        return false;

    if (!systemBusIFace->isServiceRegistered(kDaemonService))
        return false;

    // check if MountControl interface exists
    QDBusInterface daemonIntroIface(kDaemonService, kDaemonPath, kDaemonIntro,
                                    QDBusConnection::systemBus());
    QDBusReply<QString> reply = daemonIntroIface.call(kDaemonIntroMethod);
    if (reply.value().contains(R"(<node name="MountControl"/>)")) {
        // check if "SupportedFileSystems" method exists
        QDBusInterface introIface(kDaemonService,
                                  kMountControlPath,
                                  kDaemonIntro,
                                  QDBusConnection::systemBus());
        QDBusReply<QString> ifaceDesc = introIface.call(kDaemonIntroMethod);
        if (!ifaceDesc.value().contains(R"(<method name="SupportedFileSystems">)"))
            return true;

        QDBusInterface mountIface(kDaemonService,
                                  kMountControlPath,
                                  kMountControlIFace,
                                  QDBusConnection::systemBus());
        QDBusReply<QStringList> supported = mountIface.call("SupportedFileSystems");
        return supported.value().contains("cifs");
    }
    return false;
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

    // 在使用完query后立即释放
    if (query) {
        g_hash_table_unref(query);
        query = nullptr;
    }

    while (items) {
        auto item = static_cast<SecretItem *>(items->data);
        GHashTable *itemAttrs = secret_item_get_attributes(item);
        QVariantMap attr;
        g_hash_table_foreach(
                itemAttrs,
                [](gpointer k, gpointer v, gpointer vm) {
                    auto info = static_cast<QVariantMap *>(vm);
                    if (!info)
                        return;
                    info->insert(static_cast<char *>(k), QString(static_cast<char *>(v)));
                    qInfo() << "found saved login info:" << *info;
                },
                &attr);
        if (attr.contains(kSchemaDomain) && attr.contains(kSchemaProtocol)
            && attr.contains(kSchemaServer) && attr.contains(kSchemaUser))
            passwds.append(attr);
        else
            qInfo() << "got invalid saved keyring, ignore." << attr;

        // 手动释放 GHashTable 以避免内存泄漏
        if (itemAttrs)
            g_hash_table_unref(itemAttrs);

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
        else {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            // since daemon accept base64-ed passwd to mount cifs, cleartext should be encoded with base64
            // see commit of dde-file-manager: 3b50664d4034754b15c1a516cfaab8c7fbdd3db9
            passwd.insert(kLoginPasswd, QString(QByteArray(pwd).toBase64()));
#else
            passwd.insert(kLoginPasswd, QString(pwd));
#endif
        }
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
    if (isMountByDae(address))
        mountByDaemon(address, getPassInfo, mountResult, secs);
    else
        mountByGvfs(address, getPassInfo, getUserChoice, mountResult, secs);
}

bool DNetworkMounter::isMountByDae(const QString &address)
{
    QUrl u(address);
    // don't mount samba's root by Daemon
    return u.scheme() == "smb" && !u.path().remove("/").isEmpty() && isDaemonMountEnable();
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
    struct UnmountResult
    {
        bool success;
        int errnoFromDaemon;
        QString errMsgFromDaemon;
    };

    QFutureWatcher<UnmountResult> *watcher { new QFutureWatcher<UnmountResult>() };
    QObject::connect(watcher, &QFutureWatcher<UnmountResult>::finished, [cb, watcher] {
        UnmountResult result = watcher->result();
        watcher->deleteLater();
        if (cb) {
            if (result.errnoFromDaemon == -8) {   // service_mountcontrol::MountErrorCode::kAuthenticationFailed
                cb(result.success, Utils::genOperateErrorInfo(DeviceError::kUserErrorAuthenticationFailed, result.errMsgFromDaemon));
            } else if (result.errnoFromDaemon == static_cast<int>(DeviceError::kGDBusErrorNoReply)) {
                cb(result.success, Utils::genOperateErrorInfo(DeviceError::kUDisksErrorNotAuthorizedDismissed, result.errMsgFromDaemon));
            } else {
                cb(result.success,
                   Utils::genOperateErrorInfo(result.success ? DeviceError::kNoError : DeviceError::kUserError, result.errMsgFromDaemon));
            }
        }
    });

    watcher->setFuture(QtConcurrent::run([mpt]() -> UnmountResult {
        int errnoFromDaemon = 0;
        QString errMsgFromDaemon;
        bool success = unmountNetworkDevAsyncDetailed(mpt, &errnoFromDaemon, &errMsgFromDaemon);
        return { success, errnoFromDaemon, errMsgFromDaemon };
    }));
}

bool DNetworkMounter::unmountNetworkDevAsyncDetailed(const QString &mpt, int *errCode, QString *errMsg)
{
    QDBusInterface mntCtrl(kDaemonService, kMountControlPath, kMountControlIFace,
                           QDBusConnection::systemBus());
    QVariantMap opts { { kMountFsType, "cifs" } };
    QDBusReply<QVariantMap> ret = mntCtrl.call(kMountControlUnmount, mpt, opts);

    if (!ret.isValid()) {
        const auto &e = ret.error();
        const QString name = e.name();
        if (e.type() == QDBusError::NoReply || name == QStringLiteral("org.freedesktop.DBus.Error.NoReply")) {
            if (errCode) *errCode = static_cast<int>(DFMMOUNT::DeviceError::kGDBusErrorNoReply);
            if (errMsg) *errMsg = QStringLiteral("Authorization pending (NoReply)");
            return false;
        }

        if (errCode) *errCode = EINVAL;
        if (errMsg) *errMsg = "DBus call failed";
        return false;
    }

    auto result = ret.value();
    if (errCode) *errCode = result.value("errno", 0).toInt();
    if (errMsg) *errMsg = result.value("errMsg", "").toString();
    return result.value("result", false).toBool();
}

/*!
 * \brief DNetworkMounter::mountByDaemon, mount network device (smb/ftp/webdav) by
 * dde-file-manager-daemon. \param address \param getPassInfo \param mountResult \param msecs
 */
void DNetworkMounter::mountByDaemon(const QString &address, GetMountPassInfo getPassInfo,
                                    DeviceOperateCallbackWithMessage mountResult, int secs)
{
    auto requestLoginInfo = [address, getPassInfo] {
        if (getPassInfo) {
            QSettings setting(kSmbConfigPath, QSettings::IniFormat);
            return getPassInfo(QObject::tr("need authorization to access %1").arg(address),
                               Utils::currentUser(), setting.value("global/workgroup", "WORKGROUP").toString());
        }
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
            mountResult(false, Utils::genOperateErrorInfo(DeviceError::kGIOErrorAlreadyMounted), mpt);
        return;
    }

    auto logins = loginPasswd(addr);
    MountPassInfo loginInfo;
    if (logins.isEmpty()) {
        loginInfo = requestLoginInfo();
        if (loginInfo.cancelled && mountResult) {
            checkThread();
            mountResult(false, Utils::genOperateErrorInfo(DeviceError::kUserErrorUserCancelled), "");
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
                mountResult(false, Utils::genOperateErrorInfo(DeviceError::kUserErrorUserCancelled), "");
                return;
            }
            doLastMount(addr, loginInfo, mountResult);
        } else {
            if (mountResult) {
                checkThread();
                mountResult(mntRet.ok, Utils::genOperateErrorInfo(mntRet.err), mntRet.mpt);
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
                                  DeviceOperateCallbackWithMessage mountResult, int secs)
{
    QUrl url(address);
    QString pureAddr = url.toString();
    pureAddr.remove("?" + url.query());

    QString mountAddr = address;
    if (address.startsWith("ftp") && secs > 0 && !address.contains("socket_timeout=")) {
        mountAddr += (url.query().isEmpty() ? QString("?socket_timeout=%1").arg(secs)   // address = ftp://1.2.3.4
                                            : QString("&socket_timeout=%1").arg(secs));   // address = ftp://1.2.3.4?charset=utf8
    }

    qInfo() << "protocol: the mountAddress is: " << mountAddr << "and pureAddress is: " << pureAddr;

    GFile_autoptr file = g_file_new_for_uri(mountAddr.toStdString().c_str());
    if (!file) {
        qWarning() << "protocol: cannot generate location for" << mountAddr;
        return;
    }

    AskPasswdHelper *passwdHelper = new AskPasswdHelper();
    passwdHelper->callback = getPassInfo;
    passwdHelper->callOnceFlag = false;   // make sure the signal will not emit continuously when validate failed.

    AskQuestionHelper *questionHelper = new AskQuestionHelper();
    questionHelper->callback = getUserChoice;

    GMountOperation_autoptr op = g_mount_operation_new();
    g_signal_connect(op, "ask_question",
                     G_CALLBACK(DNetworkMounter::mountByGvfsAskQuestion), questionHelper);
    g_signal_connect(op, "ask_password",
                     G_CALLBACK(DNetworkMounter::mountByGvfsAskPasswd), passwdHelper);

    FinalizeHelper *finalizeHelper = new FinalizeHelper;
    finalizeHelper->askPasswd = passwdHelper;
    finalizeHelper->askQuestion = questionHelper;
    finalizeHelper->resultCallback = mountResult;
    finalizeHelper->customData = pureAddr;

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

    OperationErrorInfo derr = Utils::genOperateErrorInfo(finalize->askPasswd->err);
    auto file = reinterpret_cast<GFile *>(srcObj);
    GError_autoptr err = nullptr;
    bool ok = g_file_mount_enclosing_volume_finish(file, res, &err);
    if (!ok && derr.code == DeviceError::kNoError && err) {
        derr.code = Utils::castFromGError(err);
        derr.message = err->message;
    }

    GFile_autoptr srcFile { nullptr };
    if (finalize->customData.isValid())
        srcFile = g_file_new_for_uri(finalize->customData.toString().toStdString().c_str());
    if (srcFile)
        file = srcFile;

    g_autofree char *mntPath = g_file_get_path(file);
    GError_autoptr mountErr = nullptr;
    GMount_autoptr mount = g_file_find_enclosing_mount(file, nullptr, &mountErr);
    if (mount) {
        GFile_autoptr defLocation = g_mount_get_default_location(mount);
        if (defLocation) {
            if (mntPath) g_free(mntPath);
            mntPath = g_file_get_path(defLocation);
        }
    }
    if (finalize->resultCallback)
        finalize->resultCallback(ok, derr, mntPath);

    delete finalize->askPasswd;
    delete finalize->askQuestion;
    delete finalize;
}

static QVariant preparePasswd(const QString &passwd)
{
    if (passwd.isEmpty()) {
        qDebug() << "Created empty QVariant for empty passwd";
        return QVariant("");
    }

    // Prepare passwd
    const QByteArray passwdBytes = passwd.toLocal8Bit();

    // Create pipe
    int pipefds[2];
    if (pipe(pipefds) == -1) {
        qCritical() << "Failed to create pipe:" << strerror(errno);
        return QVariant("");
    }

    // pipefds[0] is for reading
    // pipefds[1] is for writing
    int read_fd = pipefds[0];
    int write_fd = pipefds[1];

    // Write passwd to pipe
    qint64 bytesWritten = write(write_fd, passwdBytes.constData(), passwdBytes.size());
    close(write_fd);
    if (bytesWritten != passwdBytes.size()) {
        qCritical() << "Failed to write passwd to pipe.";
        close(read_fd);
        return QVariant("");
    }

    // Create file descriptor wrapper
    QDBusUnixFileDescriptor dbusFd(read_fd);
    // read_fd has been copied to QDBusUnixFileDescriptor
    close(read_fd);

    qDebug() << "Successfully created fd for passwd transmission";
    return QVariant::fromValue(dbusFd);
}

DNetworkMounter::MountRet DNetworkMounter::mountWithUserInput(const QString &address,
                                                              const MountPassInfo info)
{
    QVariantMap param { { kLoginUser, info.userName },
                        { kLoginDomain, info.domain },
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                        { kLoginPasswd, info.passwd },
#else
                        { kLoginPasswd, preparePasswd(info.passwd) },
#endif
                        { kLoginTimeout, info.timeout },
                        { kMountFsType, "cifs" } };

    QDBusInterface mntCtrl(kDaemonService, kMountControlPath, kMountControlIFace,
                           QDBusConnection::systemBus());

    QDBusReply<QVariantMap> ret = mntCtrl.call(kMountControlMount, address, param);
    auto mntRet = ret.value();
    QString mpt = mntRet.value(kDaemonMountRetKeyMpt).toString();
    int errNum = mntRet.value(kDaemonMountRetKeyErrno).toInt();

    bool ok = !mpt.isEmpty();
    DeviceError err = (info.anonymous && errNum == EACCES)
            ? DeviceError::kUserErrorNetworkAnonymousNotAllowed
            : static_cast<DeviceError>(errNum);
    if (ok) {
        err = DeviceError::kNoError;

        if (!info.anonymous && info.savePasswd != NetworkMountPasswdSaveMode::kNeverSavePasswd) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            // since passwd from user input is base64-ed data, so the passwd should be decoded into cleartext for saving.
            // associated commit of dde-file-manager: 3b50664d4034754b15c1a516cfaab8c7fbdd3db9
            auto _info = info;
            _info.passwd = QByteArray::fromBase64(info.passwd.toLocal8Bit());
            savePasswd(address, _info);
#else
            savePasswd(address, info);
#endif
        }
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
        QVariantMap param { { kLoginUser, login.value(kSchemaUser, "") },
                            { kLoginDomain, login.value(kSchemaDomain, "") },
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                            { kLoginPasswd, login.value(kLoginPasswd, "") },
#else
                            { kLoginPasswd, preparePasswd(login.value(kLoginPasswd, "").toString()) },
#endif
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
            cb(mntRet.ok, Utils::genOperateErrorInfo(mntRet.err), mntRet.mpt);
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
        QRegularExpression reg("^/(?:run/)?media/(.*)/smbmounts/");
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
