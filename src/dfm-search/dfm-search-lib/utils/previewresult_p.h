// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PREVIEWRESULT_P_H
#define PREVIEWRESULT_P_H

#include <QSharedData>
#include <QString>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class PreviewResultPrivate : public QSharedData
{
public:
    PreviewResultPrivate() = default;
    PreviewResultPrivate(const PreviewResultPrivate &other) = default;

    QString content;         ///< Extracted content snippet
    int keywordOffset = -1;  ///< Position of keyword in full content (-1 if none/not matched)
};

DFM_SEARCH_END_NS

#endif   // PREVIEWRESULT_P_H
