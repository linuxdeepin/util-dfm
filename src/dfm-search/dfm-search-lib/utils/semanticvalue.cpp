// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "semanticvalue_p.h"

#include <dfm-search/semantic_types.h>

DFM_SEARCH_BEGIN_NS

// ── MatchSpan ──────────────────────────────────────────────────────

MatchSpan::MatchSpan()
    : d(new MatchSpanPrivate)
{
}

MatchSpan::~MatchSpan() = default;
MatchSpan::MatchSpan(const MatchSpan &other) = default;
MatchSpan &MatchSpan::operator=(const MatchSpan &other) = default;
MatchSpan::MatchSpan(MatchSpan &&other) noexcept = default;
MatchSpan &MatchSpan::operator=(MatchSpan &&other) noexcept = default;

int MatchSpan::start() const { return d->start; }
void MatchSpan::setStart(int start) { d->start = start; }
int MatchSpan::end() const { return d->end; }
void MatchSpan::setEnd(int end) { d->end = end; }
QString MatchSpan::ruleId() const { return d->ruleId; }
void MatchSpan::setRuleId(const QString &ruleId) { d->ruleId = ruleId; }

bool MatchSpan::isValid() const
{
    return d->start >= 0 && d->end > d->start;
}

// ── TimeConstraint ─────────────────────────────────────────────────

TimeConstraint::TimeConstraint()
    : d(new TimeConstraintPrivate)
{
}

TimeConstraint::~TimeConstraint() = default;
TimeConstraint::TimeConstraint(const TimeConstraint &other) = default;
TimeConstraint &TimeConstraint::operator=(const TimeConstraint &other) = default;
TimeConstraint::TimeConstraint(TimeConstraint &&other) noexcept = default;
TimeConstraint &TimeConstraint::operator=(TimeConstraint &&other) noexcept = default;

TimeConstraintKind TimeConstraint::kind() const { return d->kind; }
void TimeConstraint::setKind(TimeConstraintKind kind) { d->kind = kind; }
TimePreset TimeConstraint::preset() const { return d->preset; }
void TimeConstraint::setPreset(TimePreset preset) { d->preset = preset; }
int TimeConstraint::relativeValue() const { return d->relativeValue; }
void TimeConstraint::setRelativeValue(int value) { d->relativeValue = value; }
TimeUnit TimeConstraint::relativeUnit() const { return d->relativeUnit; }
void TimeConstraint::setRelativeUnit(TimeUnit unit) { d->relativeUnit = unit; }
QDateTime TimeConstraint::customStart() const { return d->customStart; }
void TimeConstraint::setCustomStart(const QDateTime &start) { d->customStart = start; }
QDateTime TimeConstraint::customEnd() const { return d->customEnd; }
void TimeConstraint::setCustomEnd(const QDateTime &end) { d->customEnd = end; }
TimeField TimeConstraint::timeField() const { return d->timeField; }
void TimeConstraint::setTimeField(TimeField field) { d->timeField = field; }

bool TimeConstraint::isValid() const
{
    return d->kind != TimeConstraintKind::None;
}

// ── SizeConstraint ─────────────────────────────────────────────────

SizeConstraint::SizeConstraint()
    : d(new SizeConstraintPrivate)
{
}

SizeConstraint::~SizeConstraint() = default;
SizeConstraint::SizeConstraint(const SizeConstraint &other) = default;
SizeConstraint &SizeConstraint::operator=(const SizeConstraint &other) = default;
SizeConstraint::SizeConstraint(SizeConstraint &&other) noexcept = default;
SizeConstraint &SizeConstraint::operator=(SizeConstraint &&other) noexcept = default;

qint64 SizeConstraint::minSize() const { return d->minSize; }
void SizeConstraint::setMinSize(qint64 size) { d->minSize = size; }
qint64 SizeConstraint::maxSize() const { return d->maxSize; }
void SizeConstraint::setMaxSize(qint64 size) { d->maxSize = size; }
bool SizeConstraint::includeLower() const { return d->includeLower; }
void SizeConstraint::setIncludeLower(bool include) { d->includeLower = include; }
bool SizeConstraint::includeUpper() const { return d->includeUpper; }
void SizeConstraint::setIncludeUpper(bool include) { d->includeUpper = include; }

bool SizeConstraint::isValid() const
{
    return d->minSize > 0 || d->maxSize > 0;
}

DFM_SEARCH_END_NS
