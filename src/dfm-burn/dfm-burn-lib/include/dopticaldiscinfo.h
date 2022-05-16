/*
 * Copyright (C) 2020 ~ 2021 Deepin Technology Co., Ltd.
 *
 * Author:     zhangsheng <zhangsheng@uniontech.com>
 *
 * Maintainer: zhangsheng <zhangsheng@uniontech.com>
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
#ifndef DOPTICALDISCINFO_H
#define DOPTICALDISCINFO_H

#include "dburn_global.h"

#include <QObject>
#include <QString>
#include <QSharedDataPointer>

DFM_BURN_BEGIN_NS

class DOpticalDiscInfoPrivate;

class DOpticalDiscInfo
{
    friend class DOpticalDiscInfoPrivate;
    friend class DOpticalDiscManager;

public:
    DOpticalDiscInfo(const DOpticalDiscInfo &info);
    DOpticalDiscInfo &operator=(const DOpticalDiscInfo &info);
    ~DOpticalDiscInfo();

    bool blank() const;
    QString device() const;
    QString volumeName() const;
    quint64 usedSize() const;
    quint64 availableSize() const;
    quint64 totalSize() const;
    quint64 dataBlocks() const;
    MediaType mediaType() const;
    QStringList writeSpeed() const;

protected:
    DOpticalDiscInfo();
    DOpticalDiscInfo(const QString &dev);

protected:
    QSharedDataPointer<DOpticalDiscInfoPrivate> d_ptr;
};

DFM_BURN_END_NS

#endif   // DOPTICALDISCINFO_H
