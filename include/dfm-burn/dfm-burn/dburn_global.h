// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    kDVD_R_DL,
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
