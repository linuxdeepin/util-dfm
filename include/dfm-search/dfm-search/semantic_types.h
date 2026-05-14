// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief Represents a consumed span in the input text matched by a rule.
 */
struct MatchSpan
{
    int start = -1;
    int end = -1;
    QString ruleId;

    bool isValid() const { return start >= 0 && end > start; }
};

/**
 * @brief Enum for preset time periods.
 */
enum class TimePreset {
    Today,
    Yesterday,
    DayBeforeYesterday,
    ThisWeek,
    LastWeek,
    ThisMonth,
    LastMonth,
    ThisYear,
    LastYear
};

/**
 * @brief Enum for time constraint kinds.
 */
enum class TimeConstraintKind {
    None,   ///< No time constraint
    Preset,   ///< Preset period (today, yesterday, etc.)
    Relative,   ///< Relative time (last N days/hours)
    Custom   ///< Custom datetime range
};

/**
 * @brief Represents a parsed time constraint from natural language.
 */
struct TimeConstraint
{
    TimeConstraintKind kind = TimeConstraintKind::None;
    TimePreset preset = TimePreset::Today;
    int relativeValue = 0;
    TimeUnit relativeUnit = TimeUnit::Days;
    QDateTime customStart;
    QDateTime customEnd;
    TimeField timeField = TimeField::Unspecified;   // Set by ActionExtractor; Unspecified = no action specified

    bool isValid() const { return kind != TimeConstraintKind::None; }
};

/**
 * @brief Represents a parsed size constraint from natural language.
 */
struct SizeConstraint
{
    qint64 minSize = 0;    // Minimum size in bytes (0 = no lower bound)
    qint64 maxSize = 0;    // Maximum size in bytes (0 = no upper bound)
    bool includeLower = true;
    bool includeUpper = true;

    bool isValid() const { return minSize > 0 || maxSize > 0; }
};

/**
 * @brief Represents the parsed intent from natural language input.
 *
 * This is the intermediate representation between NLP parsing
 * and search query construction. Declared public for future
 * structured API extensibility.
 */
struct ParsedIntent
{
    TimeConstraint timeConstraint;
    SizeConstraint sizeConstraint;
    QStringList fileExtensions;
    QStringList searchDirectories;   // Absolute paths resolved from location words
    bool includeHidden = false;      // true for trash (hidden directory)
    QStringList keywords;
    QList<MatchSpan> consumedSpans;
};

DFM_SEARCH_END_NS

#endif   // SEMANTIC_TYPES_H
