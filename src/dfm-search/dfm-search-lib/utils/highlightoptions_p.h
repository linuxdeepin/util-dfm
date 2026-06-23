// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HIGHLIGHTOPTIONS_P_H
#define HIGHLIGHTOPTIONS_P_H

#include <QSharedData>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class HighlightOptionsPrivate : public QSharedData
{
public:
    HighlightOptionsPrivate() = default;
    HighlightOptionsPrivate(const HighlightOptionsPrivate &other) = default;

    int maxPreviewLength = 200;       ///< Maximum snippet length in characters
    int positioningMaxLength = 30;     ///< Keyword positioning window size (min 30)
    bool enableHtml = false;           ///< Wrap matched keywords with <b> tags
};

DFM_SEARCH_END_NS

#endif   // HIGHLIGHTOPTIONS_P_H
