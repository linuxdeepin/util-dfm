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
#ifndef DFMMOUNT_GLOBAL_H
#define DFMMOUNT_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DFMMOUNT_LIBRARY)
#  define DFMMOUNTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define DFMMOUNTSHARED_EXPORT Q_DECL_IMPORT
#endif

#define DFM_MOUNT_NAMESPACE dfm_mount
#define DFM_MOUNT_BEGIN_NS namespace DFM_MOUNT_NAMESPACE {
#define DFM_MOUNT_END_NS }
#define DFM_MOUNT_USE_NS using namespace DFM_MOUNT_NAMESPACE;

DFM_MOUNT_BEGIN_NS

enum DeviceType {
    AllDevice      = 0,
    BlockDevice    = 1,    // normal block devices, like removable disks
    ProtocolDevice = 2,    // protocol devices, like smb/mtp/ftp/ptp etc...
    NetDevice      = 3,
};

enum MonitorStatus {
    Monitoring  = 0,
    Idle        = 1,

    NotDefined  = 0xff,
};

enum Property {
    // blocks property
    BlockProperty = 0,

    // protocol property
    ProtoProperty = 200,
};

DFM_MOUNT_END_NS
#endif // DFMMOUNT_GLOBAL_H
