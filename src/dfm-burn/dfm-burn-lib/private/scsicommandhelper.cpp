// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scsicommandhelper.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

DFM_BURN_BEGIN_NS

#define CHECK_CONDITION 0x01
#define ERRCODE(s) ((((s)[2] & 0x0F) << 16) | ((s)[12] << 8) | ((s)[13]))
static const int Dir_xlate[4] = {   // should have been defined
    // private in USE_SG_IO scope,
    // but it appears to be too
    0,   // implementation-dependent...
    SG_DXFER_TO_DEV,   // 1,CGC_DATA_WRITE
    SG_DXFER_FROM_DEV,   // 2,CGC_DATA_READ
    SG_DXFER_NONE
};   // 3,CGC_DATA_NONE

ScsiCommandHelper::ScsiCommandHelper(const QString &dev)
{
    fd = ::open(dev.toLocal8Bit().data(), O_RDWR | O_NONBLOCK);
}

ScsiCommandHelper::~ScsiCommandHelper()
{
    if (fd > -1)
        close(fd);
}

unsigned char &ScsiCommandHelper::operator[](size_t i)
{
    if (i == 0) {
        memset(&cgc, 0, sizeof(cgc)), memset(&_sense, 0, sizeof(_sense));
        cgc.quiet = 1;
        cgc.sense = &_sense.s;
        memset(&sg_io, 0, sizeof(sg_io));
        sg_io.interface_id = 'S';
        sg_io.mx_sb_len = sizeof(_sense);
        sg_io.cmdp = cgc.cmd;
        sg_io.sbp = _sense.u;
        sg_io.flags = SG_FLAG_LUN_INHIBIT | SG_FLAG_DIRECT_IO;
    }
    sg_io.cmd_len = i + 1;
    return cgc.cmd[i];
}

bool ScsiCommandHelper::transport(ScsiCommandHelper::Direction dir, void *buf, size_t sz)
{
    int errorCode { 0 };

    sg_io.dxferp = buf;
    sg_io.dxfer_len = sz;
    sg_io.dxfer_direction = Dir_xlate[dir];
    if (ioctl(fd, SG_IO, &sg_io))
        return -1;

    if ((sg_io.info & SG_INFO_OK_MASK) != SG_INFO_OK) {
        errno = EIO;
        errorCode = -1;
        if (sg_io.masked_status & CHECK_CONDITION) {
            errorCode = ERRCODE(_sense.u);
            if (errorCode == 0)
                errorCode = -1;
        }
    }
    return errorCode == 0;
}

DFM_BURN_END_NS
