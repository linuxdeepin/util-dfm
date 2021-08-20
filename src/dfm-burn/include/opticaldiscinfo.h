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
#ifndef OPTICALDISCINFO_H
#define OPTICALDISCINFO_H

#include <QObject>
#include <QString>
#include <QSharedData>

#include "dfm_burn_namespace.h"

BEGIN_BURN_NAMESPACE

class OpticalDiscInfoPrivate;

class OpticalDiscInfo
{
    friend class OpticalDiscInfoPrivate;
public:
    OpticalDiscInfo();
    OpticalDiscInfo(const QString &dev);
    OpticalDiscInfo(const OpticalDiscInfo &info);

    ~OpticalDiscInfo();
    OpticalDiscInfo &operator=(const OpticalDiscInfo &info);

    void setDevice(const QString &dev);

    bool blank() const;
    QString device() const;
    QString volumeName() const;
    quint64 usedSize() const;
    quint64 availableSize() const;
    quint64 totalSize() const;
    dfmburn::FileSystem fileSystem() const;
    dfmburn::MediaType mediaType() const;

protected:
    QSharedDataPointer<OpticalDiscInfoPrivate> d_ptr;
};

END_BURN_NAMESPACE

#endif // OPTICALDISCINFO_H
