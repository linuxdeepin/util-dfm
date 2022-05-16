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
#ifndef OPTICALDISCINFO_P_H
#define OPTICALDISCINFO_P_H

#include "dburn_global.h"
#include "dxorrisoengine.h"

#include <QSharedData>

#include <memory>

DFM_BURN_BEGIN_NS

class DOpticalDiscInfoPrivate : public QSharedData
{
public:
    inline DOpticalDiscInfoPrivate()
        : QSharedData()
    {
    }

    inline explicit DOpticalDiscInfoPrivate(const QString &dev)
        : isoEngine(std::make_unique<DXorrisoEngine>()), devid(dev)
    {
        initData();
    }

    inline DOpticalDiscInfoPrivate(const DOpticalDiscInfoPrivate &copy)
        : QSharedData(copy)
    {
    }

    inline ~DOpticalDiscInfoPrivate() {}

    void initData();

    std::unique_ptr<DXorrisoEngine> const isoEngine;

    /** \brief True when the media in device is blank or false otherwise.*/
    bool formatted {};
    /** \brief Type of media currently in the device.*/
    MediaType media;
    /** \brief Size of on-disc data in bytes.*/
    quint64 data {};
    /** \brief Available size in bytes.*/
    quint64 avail {};
    /** \brief Size of on-disc data in number of blocks.*/
    quint64 datablocks {};
    /** \brief List of write speeds supported, e.g. "10.0x".*/
    QStringList writespeed {};
    /** \brief Device identifier. Empty if the device property is invalid.*/
    QString devid {};
    /** \brief Volume name of the disc.*/
    QString volid {};
};

DFM_BURN_END_NS

#endif   // OPTICALDISCINFO_P_H
