// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCSICOMMANDHELPER_H
#define SCSICOMMANDHELPER_H

#include <dfm-burn/dburn_global.h>

#include <unistd.h>
#include <linux/cdrom.h>
#include <scsi/sg.h>

/*!
 * \brief copy from  dvd+rw-tools
 * HomePage: http://fy.chalmers.se/~appro/linux/DVD+RW/
 */

#define CGC_DATA_WRITE 1
#define CGC_DATA_READ 2
#define CGC_DATA_NONE 3

DFM_BURN_BEGIN_NS

class ScsiCommandHelper
{
public:
    typedef enum { kNONE = CGC_DATA_NONE,   // 3
                   kREAD = CGC_DATA_READ,   // 2
                   kWRITE = CGC_DATA_WRITE   // 1
    } Direction;

    ScsiCommandHelper(const QString &dev);
    ~ScsiCommandHelper();
    unsigned char &operator[](size_t i);
    bool transport(Direction dir = kNONE, void *buf = nullptr, size_t sz = 0);

private:
    struct cdrom_generic_command cgc;
    union {
        struct request_sense s;
        unsigned char u[18];
    } _sense;
    struct sg_io_hdr sg_io;
    int fd { -1 };
};

DFM_BURN_END_NS

#endif   // SCSICOMMANDHELPER_H
