/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
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
#ifndef DNETWORKMOUNTER_H
#define DNETWORKMOUNTER_H

#include "base/dmount_global.h"
#include "dprotocoldevice.h"

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
    static void mountNetworkDev(const QString &, GetMountPassInfo, GetUserChoice, DeviceOperateCallbackWithMessage, int);
    static bool unmountNetworkDev(const QString &);
    static void unmountNetworkDevAsync(const QString &, DeviceOperateCallback);

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
    static MountRet mountWithSavedInfos(const QString &address, const QList<QVariantMap> &infos);
    static void doLastMount(const QString &address, const MountPassInfo info, DeviceOperateCallbackWithMessage cb);

    static bool isMounted(const QString &address, QString &mpt);
};

DFM_MOUNT_END_NS

#endif   // DNETWORKMOUNTER_H
