// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-burn/dopticaldiscinfo.h>

#include "private/dopticaldiscinfo_p.h"
#include "private/scsicommandhelper.h"

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
    return d_ptr->total;
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
    total = data + avail;

    // 如果是没有 iso 镜像树的 dvd-rw 光盘，则可能是 udf 文件系统，udf 文件系统无法通过 xorriso 查询容量
    if (media == MediaType::kDVD_RW && !isoEngine->hasISOTree()) {
        auto capacity { acquireDVDRWCapacity() };
        if (capacity != 0) {
            total = capacity;
        }
    }

    formatted = isoEngine->mediaFormattedProperty();
    volid = isoEngine->mediaVolIdProperty();
    writespeed = isoEngine->mediaSpeedProperty();
    isoEngine->clearResult();
    isoEngine->releaseDevice();
}

quint64 DOpticalDiscInfoPrivate::acquireDVDRWCapacity() const
{
    quint64 totalSize { 0 };
    ScsiCommandHelper cmd { devid };
    unsigned char formats[260] { 0 };

    cmd[0] = 0x23;   // READ FORMAT CAPACITIES
    cmd[8] = 12;
    cmd[9] = 0;
    if (!cmd.transport(ScsiCommandHelper::kREAD, formats, 12)) {
        qWarning() << "cannot read dvd-rw capacity";
        return totalSize;
    }

    int len { formats[3] };
    if (len & 7 || len < 16) {
        qWarning() << "allocation length isn't sane:" << len;
        return totalSize;
    }

    cmd[0] = 0x23;   // READ FORMAT CAPACITIES
    cmd[7] = (4 + len) >> 8;   // now with real length...
    cmd[8] = (4 + len) & 0xFF;
    cmd[9] = 0;

    if (!cmd.transport(ScsiCommandHelper::kREAD, formats, static_cast<size_t>(4 + len))) {
        qWarning() << "cannot read format capacities";
        return totalSize;
    }

    if (len != formats[3]) {
        qWarning() << "parameter length inconsistency";
        return totalSize;
    }

    if (len != formats[3]) {
        qWarning() << "arameter length inconsistency";
        return totalSize;
    }

    int i { 12 };
    Q_ASSERT(i < len);
    quint64 blocksize {
        static_cast<quint64>(formats[9] << 16 | formats[10] << 8 | formats[11])
    };
    quint64 capacity {
        static_cast<quint64>(
                formats[i] << 24 | formats[i + 1] << 16 | formats[i + 2] << 8 | formats[i + 3])
    };
    totalSize = blocksize * capacity;
    return totalSize;
}
