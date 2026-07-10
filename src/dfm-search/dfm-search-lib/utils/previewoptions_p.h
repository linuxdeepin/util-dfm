// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PREVIEWOPTIONS_P_H
#define PREVIEWOPTIONS_P_H

#include <QSharedData>
#include <QString>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class PreviewOptionsPrivate : public QSharedData
{
public:
    PreviewOptionsPrivate() = default;
    PreviewOptionsPrivate(const PreviewOptionsPrivate &other) = default;

    int offset = 0;            ///< Content offset: start reading from the n-th character
    int maxLength = 200;       ///< Maximum snippet length in characters
    QString keyword;           ///< Search keyword (empty = raw offset read)
};

DFM_SEARCH_END_NS

#endif   // PREVIEWOPTIONS_P_H
