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
class MatchSpanPrivate;
class TimeConstraintPrivate;
class SizeConstraintPrivate;

/**
 * @brief Represents a consumed span in the input text matched by a rule.
 *
 * Pimpl-based (QSharedDataPointer) for ABI stability.
 */
class MatchSpan
{
public:
    MatchSpan();
    ~MatchSpan();
    MatchSpan(const MatchSpan &other);
    MatchSpan &operator=(const MatchSpan &other);
    MatchSpan(MatchSpan &&other) noexcept;
    MatchSpan &operator=(MatchSpan &&other) noexcept;

    int start() const;
    void setStart(int start);
    int end() const;
    void setEnd(int end);
    QString ruleId() const;
    void setRuleId(const QString &ruleId);

    bool isValid() const;

private:
    QSharedDataPointer<MatchSpanPrivate> d;
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
 *
 * Pimpl-based (QSharedDataPointer) for ABI stability.
 */
class TimeConstraint
{
public:
    TimeConstraint();
    ~TimeConstraint();
    TimeConstraint(const TimeConstraint &other);
    TimeConstraint &operator=(const TimeConstraint &other);
    TimeConstraint(TimeConstraint &&other) noexcept;
    TimeConstraint &operator=(TimeConstraint &&other) noexcept;

    TimeConstraintKind kind() const;
    void setKind(TimeConstraintKind kind);
    TimePreset preset() const;
    void setPreset(TimePreset preset);
    int relativeValue() const;
    void setRelativeValue(int value);
    TimeUnit relativeUnit() const;
    void setRelativeUnit(TimeUnit unit);
    QDateTime customStart() const;
    void setCustomStart(const QDateTime &start);
    QDateTime customEnd() const;
    void setCustomEnd(const QDateTime &end);
    TimeField timeField() const;
    void setTimeField(TimeField field);

    bool isValid() const;

private:
    QSharedDataPointer<TimeConstraintPrivate> d;
};

/**
 * @brief Represents a parsed size constraint from natural language.
 *
 * Pimpl-based (QSharedDataPointer) for ABI stability.
 */
class SizeConstraint
{
public:
    SizeConstraint();
    ~SizeConstraint();
    SizeConstraint(const SizeConstraint &other);
    SizeConstraint &operator=(const SizeConstraint &other);
    SizeConstraint(SizeConstraint &&other) noexcept;
    SizeConstraint &operator=(SizeConstraint &&other) noexcept;

    qint64 minSize() const;
    void setMinSize(qint64 size);
    qint64 maxSize() const;
    void setMaxSize(qint64 size);
    bool includeLower() const;
    void setIncludeLower(bool include);
    bool includeUpper() const;
    void setIncludeUpper(bool include);

    bool isValid() const;

private:
    QSharedDataPointer<SizeConstraintPrivate> d;
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
