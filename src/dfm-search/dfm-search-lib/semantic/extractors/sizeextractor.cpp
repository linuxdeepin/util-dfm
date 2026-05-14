// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sizeextractor.h"

#include "../semanticruleengine.h"

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

    const QVariantMap metadata = m_engine->ruleMetadata("size", ruleId);
    const QString typeStr = metadata.value("type").toString();
    SizeConstraint sc;

    if (typeStr == "preset") {
        sc.minSize = metadata.value("min_bytes", 0).toLongLong();
        sc.maxSize = metadata.value("max_bytes", 0).toLongLong();
        if (metadata.contains("include_upper")) {
            sc.includeUpper = metadata.value("include_upper").toBool();
        }
        if (metadata.contains("include_lower")) {
            sc.includeLower = metadata.value("include_lower").toBool();
        }
    } else if (typeStr == "dynamic") {
        const QString direction = metadata.value("direction").toString();

        if (direction == "min") {
            const QString value = match.captured("value");
            const QString unit = match.captured("unit");
            qint64 bytes = parseSizeToBytes(value, unit);
            if (bytes <= 0) {
                return;
            }
            sc.minSize = bytes;
            sc.includeLower = true;
        } else if (direction == "max") {
            const QString value = match.captured("value");
            const QString unit = match.captured("unit");
            qint64 bytes = parseSizeToBytes(value, unit);
            if (bytes <= 0) {
                return;
            }
            sc.maxSize = bytes;
            sc.includeUpper = true;
        } else if (direction == "range") {
            const QString minVal = match.captured("min_val");
            const QString minUnit = match.captured("min_unit");
            const QString maxVal = match.captured("max_val");
            const QString maxUnit = match.captured("max_unit");
            qint64 minBytes = parseSizeToBytes(minVal, minUnit);
            qint64 maxBytes = parseSizeToBytes(maxVal, maxUnit);
            if (minBytes <= 0 || maxBytes <= 0) {
                return;
            }
            sc.minSize = minBytes;
            sc.maxSize = maxBytes;
        }
    }

    if (sc.isValid()) {
        intent.sizeConstraint = sc;
        MatchSpan span;
        span.start = match.capturedStart();
        span.end = match.capturedEnd();
        span.ruleId = ruleId;
        intent.consumedSpans.append(span);
    }
}

qint64 SizeExtractor::parseSizeToBytes(const QString &value, const QString &unit)
{
    bool ok = false;
    double num = value.toDouble(&ok);
    if (!ok || num <= 0) {
        return -1;
    }

    QString upperUnit = unit.toUpper();
    if (upperUnit.isEmpty()) {
        // No unit: assume bytes
        return static_cast<qint64>(num);
    }
    if (upperUnit == "B" || upperUnit == "BB") {
        return static_cast<qint64>(num);
    }
    if (upperUnit == "K" || upperUnit == "KB") {
        return static_cast<qint64>(num * 1024);
    }
    if (upperUnit == "M" || upperUnit == "MB") {
        return static_cast<qint64>(num * 1024 * 1024);
    }
    if (upperUnit == "G" || upperUnit == "GB") {
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
