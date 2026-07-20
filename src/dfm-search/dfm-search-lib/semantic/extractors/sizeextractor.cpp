// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sizeextractor.h"

#include "semantic/semanticruleengine.h"

#include <QDebug>

DFM_SEARCH_BEGIN_NS

SizeExtractor::SizeExtractor(SemanticRuleEngine *engine)
    : m_engine(engine)
{
}

SizeExtractor::~SizeExtractor() = default;

void SizeExtractor::extract(const QString &input, ParsedIntent &intent)
{
    if (!m_engine->hasGroup("size")) {
        return;
    }

    QString ruleId;
    QRegularExpressionMatch match;
    if (!m_engine->match("size", input, match, &ruleId)) {
        return;
    }

    // Size extraction runs after time/filetype. If this size span is already
    // fully consumed by an earlier, higher-confidence dimension such as a
    // year-month time expression ("2025-5"), ignore it to avoid overwriting
    // the intent with a bogus size constraint.
    for (const MatchSpan &existing : intent.consumedSpans()) {
        if (match.capturedStart() >= existing.start()
                && match.capturedEnd() <= existing.end()) {
            return;
        }
    }

    const QVariantMap metadata = m_engine->ruleMetadata("size", ruleId);
    const QString typeStr = metadata.value("type").toString();
    SizeConstraint sc;

    if (typeStr == "preset") {
        sc.setMinSize(metadata.value("min_bytes", 0).toLongLong());
        sc.setMaxSize(metadata.value("max_bytes", 0).toLongLong());
        if (metadata.contains("include_upper")) {
            sc.setIncludeUpper(metadata.value("include_upper").toBool());
        }
        if (metadata.contains("include_lower")) {
            sc.setIncludeLower(metadata.value("include_lower").toBool());
        }
    } else if (typeStr == "dynamic") {
        const QString direction = metadata.value("direction").toString();

        if (direction == "min") {
            const QString value = match.captured("value");
            const QString unit = normalizeUnit(match.captured("unit"), metadata);
            qint64 bytes = parseSizeToBytes(value, unit);
            if (bytes <= 0) {
                return;
            }
            sc.setMinSize(bytes);
            sc.setIncludeLower(true);
        } else if (direction == "max") {
            const QString value = match.captured("value");
            const QString unit = normalizeUnit(match.captured("unit"), metadata);
            qint64 bytes = parseSizeToBytes(value, unit);
            if (bytes <= 0) {
                return;
            }
            sc.setMaxSize(bytes);
            sc.setIncludeUpper(true);
        } else if (direction == "range") {
            const QString minVal = match.captured("min_val");
            const QString minUnit = normalizeUnit(match.captured("min_unit"), metadata);
            const QString maxVal = match.captured("max_val");
            const QString maxUnit = normalizeUnit(match.captured("max_unit"), metadata);
            qint64 minBytes = parseSizeToBytes(minVal, minUnit);
            qint64 maxBytes = parseSizeToBytes(maxVal, maxUnit);
            if (minBytes <= 0 || maxBytes <= 0) {
                return;
            }
            sc.setMinSize(minBytes);
            sc.setMaxSize(maxBytes);
        }
    }

    if (sc.isValid()) {
        intent.setSizeConstraint(sc);
        MatchSpan span;
        span.setStart(match.capturedStart());
        span.setEnd(match.capturedEnd());
        span.setRuleId(ruleId);
        intent.consumedSpans().append(span);
    }
}

QString SizeExtractor::normalizeUnit(const QString &rawUnit, const QVariantMap &metadata)
{
    if (rawUnit.isEmpty()) {
        return {};
    }

    const QVariantMap unitMap = metadata.value("unit_map").toMap();
    if (!unitMap.isEmpty()) {
        const QString mapped = unitMap.value(rawUnit).toString();
        if (!mapped.isEmpty()) {
            return mapped;
        }
    }

    return rawUnit;
}

qint64 SizeExtractor::parseSizeToBytes(const QString &value, const QString &unit)
{
    bool ok = false;
    double num = value.toDouble(&ok);
    if (!ok || num <= 0) {
        return -1;
    }

    const QString u = unit.toUpper();
    if (u.isEmpty() || u == "B" || u == "BB") {
        return static_cast<qint64>(num);
    }
    if (u == "K" || u == "KB") {
        return static_cast<qint64>(num * 1024);
    }
    if (u == "M" || u == "MB") {
        return static_cast<qint64>(num * 1024 * 1024);
    }
    if (u == "G" || u == "GB") {
        return static_cast<qint64>(num * 1024 * 1024 * 1024);
    }

    qWarning() << "Unknown size unit:" << unit;
    return -1;
}

QString SizeExtractor::name() const
{
    return QStringLiteral("size");
}

DFM_SEARCH_END_NS
