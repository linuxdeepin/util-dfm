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

BEGIN_BURN_NAMESPACE

OpticalDiscInfo::OpticalDiscInfo()
    : d_ptr(new OpticalDiscInfoPrivate())
{

}

OpticalDiscInfo::OpticalDiscInfo(const QString &dev)
    : d_ptr(new OpticalDiscInfoPrivate(dev))
{

}

OpticalDiscInfo::OpticalDiscInfo(const OpticalDiscInfo &info)
    : d_ptr(info.d_ptr)
{

}

OpticalDiscInfo::~OpticalDiscInfo()
{

}

OpticalDiscInfo &OpticalDiscInfo::operator=(const OpticalDiscInfo &info)
{
    d_ptr = info.d_ptr;
    return *this;
}

void OpticalDiscInfo::setDevice(const QString &dev)
{
    *this = OpticalDiscInfo(dev);
}

bool OpticalDiscInfo::blank() const
{
    return false;
}

QString OpticalDiscInfo::device() const
{
    return "";
}

QString OpticalDiscInfo::volumeName() const
{
    return "";
}

quint64 OpticalDiscInfo::usedSize() const
{
    return 0;
}

quint64 OpticalDiscInfo::availableSize() const
{
    return 0;
}

quint64 OpticalDiscInfo::totalSize() const
{
    return usedSize() + availableSize();
}

dfmburn::FileSystem OpticalDiscInfo::fileSystem() const
{
    return dfmburn::FileSystem::kNoFS;
}

dfmburn::MediaType OpticalDiscInfo::mediaType() const
{
    return dfmburn::MediaType::kNoMedia;
}

END_BURN_NAMESPACE
