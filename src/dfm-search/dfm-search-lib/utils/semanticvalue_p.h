// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTICVALUE_P_H
#define SEMANTICVALUE_P_H

#include <QDateTime>
#include <QSharedData>
#include <QString>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/semantic_types.h>

DFM_SEARCH_BEGIN_NS

class MatchSpanPrivate : public QSharedData
{
public:
    MatchSpanPrivate() = default;
    int start = -1;
    int end = -1;
    QString ruleId;
};

class TimeConstraintPrivate : public QSharedData
{
public:
    TimeConstraintPrivate() = default;
    TimeConstraintKind kind = TimeConstraintKind::None;
    TimePreset preset = TimePreset::Today;
    int relativeValue = 0;
    TimeUnit relativeUnit = TimeUnit::Days;
    QDateTime customStart;
    QDateTime customEnd;
    TimeField timeField = TimeField::Unspecified;
};

class SizeConstraintPrivate : public QSharedData
{
public:
    SizeConstraintPrivate() = default;
    qint64 minSize = 0;
    qint64 maxSize = 0;
    bool includeLower = true;
    bool includeUpper = true;
};

DFM_SEARCH_END_NS

#endif   // SEMANTICVALUE_P_H
