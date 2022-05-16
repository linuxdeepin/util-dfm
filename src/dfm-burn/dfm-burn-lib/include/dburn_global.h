/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#ifndef DBURN_GLOBAL_H
#define DBURN_GLOBAL_H

#include <QObject>

#define DFMBURN dfmburn
#define DFM_BURN_BEGIN_NS namespace DFMBURN {
#define DFM_BURN_END_NS }
#define DFM_BURN_USE_NS using namespace DFMBURN;

DFM_BURN_BEGIN_NS

enum class BurnOption : unsigned int {
    kKeepAppendable = 1,
    kVerifyDatas = 1 << 1,
    kEjectDisc = 1 << 2,   // not used yet.
    kISO9660Only = 1 << 3,   // default
    kJolietSupport = 1 << 4,   // add joliet extension
    kRockRidgeSupport = 1 << 5,   // add rockridge extension
    kUDF102Supported = 1 << 6,
    kJolietAndRockRidge = kJolietSupport | kRockRidgeSupport   // add both of them, not used yet
};
Q_DECLARE_FLAGS(BurnOptions, BurnOption)

enum class MediaType : unsigned int {
    kNoMedia,
    kCD_ROM,
    kCD_R,
    kCD_RW,
    kDVD_ROM,
    kDVD_R,
    kDVD_RW,
    kDVD_PLUS_R,
    kDVD_PLUS_R_DL,
    kDVD_RAM,
    kDVD_PLUS_RW,
    kBD_ROM,
    kBD_R,
    kBD_RE
};

enum class JobStatus : int {
    kFailed = -1,
    kIdle,
    kRunning,
    kStalled,
    kFinished
};

DFM_BURN_END_NS

#endif   // DBURN_GLOBAL_H
