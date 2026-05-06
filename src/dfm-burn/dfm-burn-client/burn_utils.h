// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BURN_UTILS_H
#define BURN_UTILS_H

#include <QString>
#include <dfm-burn/dburn_global.h>

DFM_BURN_BEGIN_NS

/**
 * @brief Format byte size to human-readable string
 * @param bytes Size in bytes
 * @return Formatted string (e.g., "702.5 MB")
 */
QString formatSize(quint64 bytes);

/**
 * @brief Convert MediaType enum to display name
 */
QString mediaTypeName(MediaType type);

/**
 * @brief Convert JobStatus enum to display name
 */
QString jobStatusName(JobStatus status);

/**
 * @brief Convert BurnOptions flags to human-readable description
 * Returns one-line summary like "UDF filesystem, multi-session, verify after burn"
 */
QString burnOptionsSummary(BurnOptions options);

/**
 * @brief Check if a media type is rewritable (can be erased)
 */
bool isRewritable(MediaType type);

DFM_BURN_END_NS

#endif   // BURN_UTILS_H
