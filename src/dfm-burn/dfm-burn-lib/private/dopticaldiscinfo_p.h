// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OPTICALDISCINFO_P_H
#define OPTICALDISCINFO_P_H

#include <dfm-burn/dburn_global.h>

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
    quint64 acquireDVDRWCapacity() const;

    std::unique_ptr<DXorrisoEngine> const isoEngine;

    /** \brief True when the media in device is blank or false otherwise.*/
    bool formatted {};
    /** \brief Type of media currently in the device.*/
    MediaType media;
    /** \brief Size of on-disc data in bytes.*/
    quint64 data {};
    /** \brief Available size in bytes.*/
    quint64 avail {};
    /** \brief Total size in bytes */
    quint64 total {};
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
