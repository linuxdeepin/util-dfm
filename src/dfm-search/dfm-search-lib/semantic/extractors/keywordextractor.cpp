// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "keywordextractor.h"

#include "semantic/semanticruleengine.h"

#include <QRegularExpression>

DFM_SEARCH_BEGIN_NS

KeywordExtractor::KeywordExtractor(SemanticRuleEngine *engine)
    : m_engine(engine)
{
}

KeywordExtractor::~KeywordExtractor() = default;

void KeywordExtractor::extract(const QString &input, ParsedIntent &intent)
{
    // Strategy 1: structured keyword patterns (e.g., "contains X and Y")
    if (extractStructuredKeywords(input, intent)) {
        return;
    }

    // Strategy 2: extract unconsumed text regions
    extractUnconsumedText(input, intent);
}

bool KeywordExtractor::extractStructuredKeywords(const QString &input, ParsedIntent &intent)
{
    if (!m_engine->hasGroup("keyword")) {
        return false;
    }

    QString ruleId;
    QRegularExpressionMatch match;
    if (!m_engine->match("keyword", input, match, &ruleId)) {
        return false;
    }

    const QVariantMap metadata = m_engine->ruleMetadata("keyword", ruleId);
    const int captureGroup = metadata.value("capture_group", 1).toInt();

    if (captureGroup <= 0 || captureGroup > match.lastCapturedIndex()) {
        return false;
    }

    QString captured = match.captured(captureGroup).trimmed();
    if (captured.isEmpty()) {
        return false;
    }

    const bool multiKeyword = metadata.value("multi_keyword", false).toBool();

    if (multiKeyword) {
        intent.keywords = splitMultiKeywords(captured, metadata);
    } else {
        intent.keywords = { captured };
    }

    // Determine search target from rule metadata
    const QString targetStr = metadata.value("search_target").toString();
    if (targetStr == "filename") {
        intent.searchTarget = SearchTarget::FileNameOnly;
    } else if (targetStr == "content") {
        intent.searchTarget = SearchTarget::ContentOnly;
    }
    // "all" or empty → keep default SearchTarget::All

    // Mark the entire matched region as consumed
    MatchSpan span;
    span.start = match.capturedStart();
    span.end = match.capturedEnd();
    span.ruleId = ruleId;
    intent.consumedSpans.append(span);

    return true;
}

void KeywordExtractor::extractUnconsumedText(const QString &input, ParsedIntent &intent)
{
    QList<MatchSpan> allSpans = intent.consumedSpans;

    // Also consume noise words
    if (m_engine->hasGroup("noise")) {
        QStringList noiseRuleIds;
        const QList<QRegularExpressionMatch> noiseMatches =
                m_engine->matchAll("noise", input, &noiseRuleIds);

        for (int i = 0; i < noiseMatches.size(); ++i) {
            MatchSpan span;
            span.start = noiseMatches[i].capturedStart();
            span.end = noiseMatches[i].capturedEnd();
            span.ruleId = noiseRuleIds[i];
            allSpans.append(span);
        }
    }

    // Extract text not covered by any consumed span
    const QString unconsumed = extractUnconsumedRegions(input, allSpans);

    if (unconsumed.isEmpty()) {
        return;
    }

    // Clean up punctuation and particles using pattern from rule metadata
    // Default: strip whitespace
    const QString cleanupPattern = QStringLiteral("[\\s]+");
    QRegularExpression cleanupRe(cleanupPattern);

    // Try to get a more specific cleanup pattern from keyword rules
    // Load from ALL rules in the group (not just matching ones),
    // since cleanup_pattern is a configuration property, not a per-match property.
    if (m_engine->hasGroup("keyword")) {
        const QStringList allRuleIds = m_engine->ruleIds("keyword");
        for (const QString &rid : allRuleIds) {
            const QVariantMap meta = m_engine->ruleMetadata("keyword", rid);
            const QString pattern = meta.value("cleanup_pattern").toString();
            if (!pattern.isEmpty()) {
                cleanupRe.setPattern(pattern);
                break;
            }
        }
    }

    const QString cleaned = unconsumed.trimmed()
                                    .replace(cleanupRe, " ")
                                    .simplified();

    if (cleaned.isEmpty()) {
        return;
    }

    intent.keywords = { cleaned };
}

QString KeywordExtractor::extractUnconsumedRegions(const QString &input, const QList<MatchSpan> &allSpans) const
{
    if (input.isEmpty()) {
        return {};
    }

    // Build a set of consumed character positions
    QVector<bool> consumed(input.size(), false);
    for (const MatchSpan &span : allSpans) {
        if (span.isValid() && span.end <= input.size()) {
            for (int i = span.start; i < span.end; ++i) {
                consumed[i] = true;
            }
        }
    }

    // Extract unconsumed regions
    QString result;
    int regionStart = -1;

    for (int i = 0; i < input.size(); ++i) {
        if (!consumed[i]) {
            if (regionStart < 0) {
                regionStart = i;
            }
        } else {
            if (regionStart >= 0) {
                result += input.mid(regionStart, i - regionStart) + " ";
                regionStart = -1;
            }
        }
    }

    // Trailing region
    if (regionStart >= 0) {
        result += input.mid(regionStart);
    }

    return result.trimmed();
}

QStringList KeywordExtractor::splitMultiKeywords(const QString &text, const QVariantMap &metadata)
{
    // Default split on comma
    QString splitPattern = QStringLiteral("[,]+");

    // Try to get language-specific split pattern from metadata
    const QString metaSplit = metadata.value("split_pattern").toString();
    if (!metaSplit.isEmpty()) {
        splitPattern = metaSplit;
    }

    QRegularExpression splitRe(splitPattern);
    const QStringList parts = text.split(splitRe, Qt::SkipEmptyParts);
    QStringList result;
    for (const QString &part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            result.append(trimmed);
        }
    }
    return result;
}

QString KeywordExtractor::name() const
{
    return QStringLiteral("keyword");
}

DFM_SEARCH_END_NS
