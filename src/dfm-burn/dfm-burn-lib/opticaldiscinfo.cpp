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
#include "opticaldiscinfo.h"
#include "private/opticaldiscinfo_p.h"

DFM_BURN_USE_NS

OpticalDiscInfo::OpticalDiscInfo()
    : d_ptr(new OpticalDiscInfoPrivate())
{
}

OpticalDiscInfo::OpticalDiscInfo(const QString &dev)
    : d_ptr(new OpticalDiscInfoPrivate(dev))
{
}

OpticalDiscInfo::~OpticalDiscInfo()
{
}

OpticalDiscInfo::OpticalDiscInfo(const OpticalDiscInfo &info)
    : d_ptr(info.d_ptr)
{
}

OpticalDiscInfo &OpticalDiscInfo::operator=(const OpticalDiscInfo &info)
{
    d_ptr = info.d_ptr;
    return *this;
}

bool OpticalDiscInfo::blank() const
{
    return d_ptr->formatted;
}

QString OpticalDiscInfo::device() const
{
    return d_ptr->devid;
}

QString OpticalDiscInfo::volumeName() const
{
    return d_ptr->volid;
}

quint64 OpticalDiscInfo::usedSize() const
{
    return d_ptr->data;
}

quint64 OpticalDiscInfo::availableSize() const
{
    return d_ptr->avail;
}

quint64 OpticalDiscInfo::totalSize() const
{
    return usedSize() + availableSize();
}

dfmburn::MediaType OpticalDiscInfo::mediaType() const
{
    return dfmburn::MediaType::kNoMedia;
}

QStringList OpticalDiscInfo::writeSpeed() const
{
    return d_ptr->writespeed;
}

void OpticalDiscInfoPrivate::initData()
{
    isoEngine->acquireDevice(devid);
    media = isoEngine->mediaTypeProperty();
    isoEngine->mediaStorageProperty(&data, &avail, &datablocks);
    formatted = isoEngine->mediaFormattedProperty();
    volid = isoEngine->mediaVolIdProperty();
    writespeed = isoEngine->mediaSpeedProperty();
    isoEngine->clearResult();
    isoEngine->releaseDevice();
}
