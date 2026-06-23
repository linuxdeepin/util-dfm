// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTIC_TYPES_H
#define SEMANTIC_TYPES_H

#include <QDateTime>
#include <QList>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class ParsedIntentPrivate;

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
 * @brief Represents the target scope for a semantic search.
 *
 * When a user explicitly specifies where to search (e.g. "文件名包含XX"
 * vs "文件内容包含XX"), this enum controls which search paths are enabled.
 */
enum class SearchTarget {
    All,          ///< All search paths enabled (default)
    FileNameOnly, ///< Only filename search
    ContentOnly   ///< Only content + OCR search
};

/**
 * @brief Represents the parsed intent from natural language input.
 *
 * This is the intermediate representation between NLP parsing
 * and search query construction. Declared public for future
 * structured API extensibility.
 *
 * Pimpl-based (QSharedDataPointer) for ABI stability:
 * new fields can be added to ParsedIntentPrivate without
 * breaking binary compatibility.
 */
class ParsedIntent
{
public:
    ParsedIntent();
    ~ParsedIntent();
    ParsedIntent(const ParsedIntent &other);
    ParsedIntent &operator=(const ParsedIntent &other);
    ParsedIntent(ParsedIntent &&other) noexcept;
    ParsedIntent &operator=(ParsedIntent &&other) noexcept;

    // timeConstraint
    const TimeConstraint &timeConstraint() const;
    TimeConstraint &timeConstraint();
    void setTimeConstraint(const TimeConstraint &tc);

    // sizeConstraint
    const SizeConstraint &sizeConstraint() const;
    SizeConstraint &sizeConstraint();
    void setSizeConstraint(const SizeConstraint &sc);

    // fileExtensions
    const QStringList &fileExtensions() const;
    QStringList &fileExtensions();
    void setFileExtensions(const QStringList &extensions);

    // searchDirectories
    const QStringList &searchDirectories() const;
    QStringList &searchDirectories();
    void setSearchDirectories(const QStringList &dirs);

    // includeHidden
    bool includeHidden() const;
    void setIncludeHidden(bool include);

    // hiddenOnly
    bool hiddenOnly() const;
    void setHiddenOnly(bool hidden);

    // keywords
    const QStringList &keywords() const;
    QStringList &keywords();
    void setKeywords(const QStringList &kw);

    // searchTarget
    SearchTarget searchTarget() const;
    void setSearchTarget(SearchTarget target);

    // consumedSpans
    const QList<MatchSpan> &consumedSpans() const;
    QList<MatchSpan> &consumedSpans();
    void setConsumedSpans(const QList<MatchSpan> &spans);

private:
    QSharedDataPointer<ParsedIntentPrivate> d;
};

DFM_SEARCH_END_NS

#endif   // SEMANTIC_TYPES_H
