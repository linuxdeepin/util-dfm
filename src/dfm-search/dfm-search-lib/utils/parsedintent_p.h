// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PARSEDINTENT_P_H
#define PARSEDINTENT_P_H

#include <QSharedData>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/semantic_types.h>

DFM_SEARCH_BEGIN_NS

class ParsedIntentPrivate : public QSharedData
{
public:
    ParsedIntentPrivate() = default;
    ParsedIntentPrivate(const ParsedIntentPrivate &other) = default;

    TimeConstraint timeConstraint;
    SizeConstraint sizeConstraint;
    QStringList fileExtensions;
    QStringList searchDirectories;
    bool includeHidden = false;
    bool hiddenOnly = false;
    bool recentOnly = false;   // true = search recently-used files (DBus RecentManager)
    QStringList keywords;
    SearchTarget searchTarget = SearchTarget::All;
    QList<MatchSpan> consumedSpans;
};

DFM_SEARCH_END_NS

#endif   // PARSEDINTENT_P_H
