/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             zhangsheng<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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

#include "dfmio_utils.h"

#include <gio/gio.h>

USING_IO_NAMESPACE

bool DFMUtils::fileUnmountable(const QString &path)
{
    g_autoptr(GFile) gfile = g_file_new_for_path(path.toLocal8Bit().data());

    g_autoptr(GMount) gmount = g_file_find_enclosing_mount(gfile, nullptr, nullptr);
    if (gmount) {
        return g_mount_can_unmount(gmount);
    }

    return false;
}
