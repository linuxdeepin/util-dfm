/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
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
#ifndef DFMIO_UTILS_H
#define DFMIO_UTILS_H

#include "dfmio_global.h"

#include <QString>

class QUrl;

BEGIN_IO_NAMESPACE

class DFMUtils
{
public:
    static bool fileUnmountable(const QString &path);
    static QString devicePathFromUrl(const QUrl &url);
    static QString fsTypeFromUrl(const QUrl &url);
    static QUrl directParentUrl(const QUrl &url, const bool localFirst = true);
    static bool fileIsRemovable(const QUrl &url);
    static QSet<QString> hideListFromUrl(const QUrl &url);
};

END_IO_NAMESPACE

#endif   // DFMIO_UTILS_H
