// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSM3HASH_H
#define DSM3HASH_H

#include <dfm-burn/dburn_global.h>

#include <QString>

DFM_BURN_BEGIN_NS

class DSM3Hash
{
public:
    /*!
     * \brief Compute SM3 hash of a file.
     * \param filePath Absolute path to the file.
     * \return Hex-encoded SM3 hash string (64 chars), or empty string on failure.
     */
    static QString sm3File(const QString &filePath);
};

DFM_BURN_END_NS

#endif   // DSM3HASH_H
