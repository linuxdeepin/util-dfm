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
#include "dopticaldiscinfo.h"
#include "private/dopticaldiscinfo_p.h"

#include <QDebug>

DFM_BURN_USE_NS

DOpticalDiscInfo::DOpticalDiscInfo()
    : d_ptr(new DOpticalDiscInfoPrivate())
{
}

DOpticalDiscInfo::DOpticalDiscInfo(const QString &dev)
    : d_ptr(new DOpticalDiscInfoPrivate(dev))
{
}

DOpticalDiscInfo::~DOpticalDiscInfo()
{
}

DOpticalDiscInfo::DOpticalDiscInfo(const DOpticalDiscInfo &info)
    : d_ptr(info.d_ptr)
{
}

DOpticalDiscInfo &DOpticalDiscInfo::operator=(const DOpticalDiscInfo &info)
{
    d_ptr = info.d_ptr;
    return *this;
}

bool DOpticalDiscInfo::blank() const
{
    return d_ptr->formatted;
}

QString DOpticalDiscInfo::device() const
{
    return d_ptr->devid;
}

QString DOpticalDiscInfo::volumeName() const
{
    return d_ptr->volid;
}

quint64 DOpticalDiscInfo::usedSize() const
{
    return d_ptr->data;
}

quint64 DOpticalDiscInfo::availableSize() const
{
    return d_ptr->avail;
}

quint64 DOpticalDiscInfo::totalSize() const
{
    return usedSize() + availableSize();
}

quint64 DOpticalDiscInfo::dataBlocks() const
{
    return d_ptr->datablocks;
}

dfmburn::MediaType DOpticalDiscInfo::mediaType() const
{
    return d_ptr->media;
}

QStringList DOpticalDiscInfo::writeSpeed() const
{
    return d_ptr->writespeed;
}

void DOpticalDiscInfoPrivate::initData()
{
    if (!isoEngine->acquireDevice(devid)) {
        qWarning() << "[dfm-burn]: Init data failed, cannot acquire device";
        devid = "";
        return;
    }
    media = isoEngine->mediaTypeProperty();
    isoEngine->mediaStorageProperty(&data, &avail, &datablocks);
    formatted = isoEngine->mediaFormattedProperty();
    volid = isoEngine->mediaVolIdProperty();
    writespeed = isoEngine->mediaSpeedProperty();
    isoEngine->clearResult();
    isoEngine->releaseDevice();
}
