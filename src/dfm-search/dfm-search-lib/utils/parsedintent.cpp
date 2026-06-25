// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "parsedintent_p.h"

#include <dfm-search/semantic_types.h>

DFM_SEARCH_BEGIN_NS

// ── ParsedIntent constructors ──────────────────────────────────────

ParsedIntent::ParsedIntent()
    : d(new ParsedIntentPrivate)
{
}

ParsedIntent::~ParsedIntent() = default;

ParsedIntent::ParsedIntent(const ParsedIntent &other) = default;

ParsedIntent &ParsedIntent::operator=(const ParsedIntent &other) = default;

ParsedIntent::ParsedIntent(ParsedIntent &&other) noexcept = default;

ParsedIntent &ParsedIntent::operator=(ParsedIntent &&other) noexcept = default;

// ── ParsedIntent accessors ──────────────────────────────────────────

// timeConstraint
const TimeConstraint &ParsedIntent::timeConstraint() const
{
    return d->timeConstraint;
}

TimeConstraint &ParsedIntent::timeConstraint()
{
    return d->timeConstraint;
}

void ParsedIntent::setTimeConstraint(const TimeConstraint &tc)
{
    d->timeConstraint = tc;
}

// sizeConstraint
const SizeConstraint &ParsedIntent::sizeConstraint() const
{
    return d->sizeConstraint;
}

SizeConstraint &ParsedIntent::sizeConstraint()
{
    return d->sizeConstraint;
}

void ParsedIntent::setSizeConstraint(const SizeConstraint &sc)
{
    d->sizeConstraint = sc;
}

// fileExtensions
const QStringList &ParsedIntent::fileExtensions() const
{
    return d->fileExtensions;
}

QStringList &ParsedIntent::fileExtensions()
{
    return d->fileExtensions;
}

void ParsedIntent::setFileExtensions(const QStringList &extensions)
{
    d->fileExtensions = extensions;
}

// searchDirectories
const QStringList &ParsedIntent::searchDirectories() const
{
    return d->searchDirectories;
}

QStringList &ParsedIntent::searchDirectories()
{
    return d->searchDirectories;
}

void ParsedIntent::setSearchDirectories(const QStringList &dirs)
{
    d->searchDirectories = dirs;
}

// includeHidden
bool ParsedIntent::includeHidden() const
{
    return d->includeHidden;
}

void ParsedIntent::setIncludeHidden(bool include)
{
    d->includeHidden = include;
}

// hiddenOnly
bool ParsedIntent::hiddenOnly() const
{
    return d->hiddenOnly;
}

void ParsedIntent::setHiddenOnly(bool hidden)
{
    d->hiddenOnly = hidden;
}

// recentOnly
bool ParsedIntent::recentOnly() const
{
    return d->recentOnly;
}

void ParsedIntent::setRecentOnly(bool recent)
{
    d->recentOnly = recent;
}

// keywords
const QStringList &ParsedIntent::keywords() const
{
    return d->keywords;
}

QStringList &ParsedIntent::keywords()
{
    return d->keywords;
}

void ParsedIntent::setKeywords(const QStringList &kw)
{
    d->keywords = kw;
}

// searchTarget
SearchTarget ParsedIntent::searchTarget() const
{
    return d->searchTarget;
}

void ParsedIntent::setSearchTarget(SearchTarget target)
{
    d->searchTarget = target;
}

// consumedSpans
const QList<MatchSpan> &ParsedIntent::consumedSpans() const
{
    return d->consumedSpans;
}

QList<MatchSpan> &ParsedIntent::consumedSpans()
{
    return d->consumedSpans;
}

void ParsedIntent::setConsumedSpans(const QList<MatchSpan> &spans)
{
    d->consumedSpans = spans;
}

DFM_SEARCH_END_NS
