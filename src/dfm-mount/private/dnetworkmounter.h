// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DNETWORKMOUNTER_H
#define DNETWORKMOUNTER_H

#include <dfm-mount/base/dmount_global.h>
#include <dfm-mount/dprotocoldevice.h>

#include <QString>
#include <QVariantMap>

extern "C" {
#include <libsecret/secret.h>
}

DFM_MOUNT_BEGIN_NS

class DNetworkMounter
{
public:
    static bool isDaemonMountEnable();
    static bool isMountByDae(const QString &address);
    static void mountNetworkDev(const QString &, GetMountPassInfo, GetUserChoice, DeviceOperateCallbackWithMessage, int);
    static bool unmountNetworkDev(const QString &);
    static void unmountNetworkDevAsync(const QString &, DeviceOperateCallback);
    static bool unmountNetworkDevAsyncDetailed(const QString &mpt, int *errCode, QString *errMsg);

private:
    static QList<QVariantMap> loginPasswd(const QString &address);
    static void savePasswd(const QString &address, const MountPassInfo &info);
    static SecretSchema *smbSchema();

    static void mountByDaemon(const QString &, GetMountPassInfo, DeviceOperateCallbackWithMessage, int);
    static void mountByGvfs(const QString &, GetMountPassInfo, GetUserChoice, DeviceOperateCallbackWithMessage, int);

    static void mountByGvfsAskQuestion(GMountOperation *self, const char *message, const char **choices, gpointer userData);
    static void mountByGvfsAskPasswd(GMountOperation *self, gchar *message, gchar *defaultUser, gchar *defaultDomain, GAskPasswordFlags flags, gpointer userData);
    static void mountByGvfsCallback(GObject *srcObj, GAsyncResult *res, gpointer userData);

    struct MountRet
    {
        bool ok { false };
        DeviceError err { DeviceError::kNoError };
        QString mpt {};
        bool requestLoginInfo { false };
    };
    static MountRet mountWithUserInput(const QString &address, const MountPassInfo info);
    static MountRet mountWithSavedInfos(const QString &address, const QList<QVariantMap> &infos, int secs = 0);
    static void doLastMount(const QString &address, const MountPassInfo info, DeviceOperateCallbackWithMessage cb);

    static bool isMounted(const QString &address, QString &mpt);
};

DFM_MOUNT_END_NS

#endif   // DNETWORKMOUNTER_H
