// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "burn_utils.h"

DFM_BURN_BEGIN_NS

QString formatSize(quint64 bytes)
{
    if (bytes == 0)
        return "0 B";

    const QStringList units = { "B", "KB", "MB", "GB", "TB" };
    int idx = 0;
    double size = bytes;

    while (size >= 1024.0 && idx < units.size() - 1) {
        size /= 1024.0;
        ++idx;
    }

    if (idx == 0)
        return QString("%1 B").arg(bytes);

    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units.at(idx));
}

QString mediaTypeName(MediaType type)
{
    switch (type) {
    case MediaType::kNoMedia:
        return "No Media";
    case MediaType::kCD_ROM:
        return "CD-ROM";
    case MediaType::kCD_R:
        return "CD-R";
    case MediaType::kCD_RW:
        return "CD-RW";
    case MediaType::kDVD_ROM:
        return "DVD-ROM";
    case MediaType::kDVD_R:
        return "DVD-R";
    case MediaType::kDVD_RW:
        return "DVD-RW";
    case MediaType::kDVD_PLUS_R:
        return "DVD+R";
    case MediaType::kDVD_PLUS_R_DL:
        return "DVD+R DL";
    case MediaType::kDVD_R_DL:
        return "DVD-R DL";
    case MediaType::kDVD_RAM:
        return "DVD-RAM";
    case MediaType::kDVD_PLUS_RW:
        return "DVD+RW";
    case MediaType::kBD_ROM:
        return "BD-ROM";
    case MediaType::kBD_R:
        return "BD-R";
    case MediaType::kBD_RE:
        return "BD-RE";
    }
    return "Unknown";
}

QString jobStatusName(JobStatus status)
{
    switch (status) {
    case JobStatus::kIdle:
        return "Idle";
    case JobStatus::kRunning:
        return "Running";
    case JobStatus::kStalled:
        return "Stalled";
    case JobStatus::kFinished:
        return "Finished";
    case JobStatus::kFailed:
        return "Failed";
    }
    return "Unknown";
}

QString burnOptionsSummary(BurnOptions options)
{
    QStringList parts;

    // Filesystem
    if (options.testFlag(BurnOption::kUDF102Supported))
        parts << "UDF filesystem";
    else if (options.testFlag(BurnOption::kJolietSupport) && options.testFlag(BurnOption::kRockRidgeSupport))
        parts << "ISO9660 + Joliet + RockRidge";
    else if (options.testFlag(BurnOption::kJolietSupport))
        parts << "ISO9660 + Joliet";
    else if (options.testFlag(BurnOption::kRockRidgeSupport))
        parts << "ISO9660 + RockRidge";
    else
        parts << "ISO9660 filesystem";

    // Behavior
    if (options.testFlag(BurnOption::kKeepAppendable))
        parts << "multi-session (disc stays open)";
    if (options.testFlag(BurnOption::kVerifyDatas))
        parts << "verify after burn";

    return parts.join(", ");
}

/**
 * @brief Check if a media type is rewritable
 */
bool isRewritable(MediaType type)
{
    switch (type) {
    case MediaType::kCD_RW:
    case MediaType::kDVD_RW:
    case MediaType::kDVD_PLUS_RW:
    case MediaType::kDVD_RAM:
    case MediaType::kBD_RE:
        return true;
    default:
        return false;
    }
}

DFM_BURN_END_NS
