// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timeextractor.h"

#include "../semanticruleengine.h"

#include <QDate>
#include <QDebug>

DFM_SEARCH_BEGIN_NS

TimeExtractor::TimeExtractor(SemanticRuleEngine *engine)
    : m_engine(engine)
{
}

TimeExtractor::~TimeExtractor() = default;

void TimeExtractor::extract(const QString &input, ParsedIntent &intent)
{
    if (!m_engine->hasGroup("time")) {
        return;
    }

    QString ruleId;
    QRegularExpressionMatch match;
    if (!m_engine->match("time", input, match, &ruleId)) {
        return;
    }

    const QVariantMap metadata = m_engine->ruleMetadata("time", ruleId);
    const QString typeStr = metadata.value("type").toString();
    TimeConstraint tc;

    if (typeStr == "preset") {
        const QString presetStr = metadata.value("preset").toString();
        static const QMap<QString, TimePreset> kPresetMap = {
            {"today", TimePreset::Today},
            {"yesterday", TimePreset::Yesterday},
            {"day_before_yesterday", TimePreset::DayBeforeYesterday},
            {"this_week", TimePreset::ThisWeek},
            {"last_week", TimePreset::LastWeek},
            {"this_month", TimePreset::ThisMonth},
            {"last_month", TimePreset::LastMonth},
            {"this_year", TimePreset::ThisYear},
            {"last_year", TimePreset::LastYear},
        };

        if (kPresetMap.contains(presetStr)) {
            tc.kind = TimeConstraintKind::Preset;
            tc.preset = kPresetMap.value(presetStr);
        }
    } else if (typeStr == "custom") {
        parseCustomTime(match, metadata, tc);
    }

    if (tc.isValid()) {
        intent.timeConstraint = tc;
        MatchSpan span;
        span.start = match.capturedStart();
        span.end = match.capturedEnd();
        span.ruleId = ruleId;
        intent.consumedSpans.append(span);
    }
}

void TimeExtractor::parseCustomTime(const QRegularExpressionMatch &match,
                                     const QVariantMap &metadata,
                                     TimeConstraint &tc)
{
    Q_UNUSED(metadata);

    auto tryCapture = [&match](const QString &name) -> QString {
        const QString val = match.captured(name);
        return val.isNull() ? QString() : val;
    };

    // Load locale-aware number conversion from rule metadata
    const QMap<QString, int> digitMap = mapFromVariant(metadata.value("digit_map"));
    const QString tensUnit = metadata.value("tens_unit").toString();

    const QDate today = QDate::currentDate();
    int year = today.year();
    int month = 0;
    int day = 0;

    {
        const QString yearStr = tryCapture("year");
        if (!yearStr.isEmpty()) {
            year = localeAwareToInt(yearStr, digitMap, tensUnit);
            if (year <= 0) {
                qWarning() << "Invalid year:" << yearStr;
                return;
            }
            if (year < 100) {
                year += 2000;
            }
        }
    }

    {
        const QString monthStr = tryCapture("month");
        if (!monthStr.isEmpty()) {
            month = localeAwareToInt(monthStr, digitMap, tensUnit);
            if (month < 1 || month > 12) {
                qWarning() << "Invalid month:" << monthStr;
                return;
            }
        }
    }

    {
        const QString dayStr = tryCapture("day");
        if (!dayStr.isEmpty()) {
            day = localeAwareToInt(dayStr, digitMap, tensUnit);
            if (day < 1 || day > 31) {
                qWarning() << "Invalid day:" << dayStr;
                return;
            }
        }
    }

    // Validate date
    if (month > 0 && day > 0) {
        QDate date(year, month, day);
        if (!date.isValid()) {
            qWarning() << "Invalid date:" << year << month << day;
            return;
        }
        tc.kind = TimeConstraintKind::Custom;
        tc.customStart = QDateTime(date, QTime(0, 0, 0));
        tc.customEnd = QDateTime(date, QTime(23, 59, 59));
    } else if (month > 0 && day == 0) {
        // Year-month only: entire month
        QDate monthStart(year, month, 1);
        QDate monthEnd(year, month, monthStart.daysInMonth());
        tc.kind = TimeConstraintKind::Custom;
        tc.customStart = QDateTime(monthStart, QTime(0, 0, 0));
        tc.customEnd = QDateTime(monthEnd, QTime(23, 59, 59));
    } else if (month == 0) {
        // Year only: entire year
        tc.kind = TimeConstraintKind::Custom;
        tc.customStart = QDateTime(QDate(year, 1, 1), QTime(0, 0, 0));
        tc.customEnd = QDateTime(QDate(year, 12, 31), QTime(23, 59, 59));
    }
}

int TimeExtractor::localeAwareToInt(const QString &input,
                                    const QMap<QString, int> &digitMap,
                                    const QString &tensUnit)
{
    if (input.isEmpty()) {
        return -1;
    }

    // Try direct integer conversion first (handles Arabic numerals)
    bool ok = false;
    int directValue = input.toInt(&ok);
    if (ok) {
        return directValue;
    }

    // Single digit character from digit_map
    if (input.size() == 1 && digitMap.contains(input)) {
        return digitMap.value(input);
    }

    // No digit_map or tens_unit configured — cannot parse locale-specific numbers
    if (digitMap.isEmpty() || tensUnit.isEmpty()) {
        return -1;
    }

    // Two-character pattern: "XY" where Y is the tens unit (e.g., "十五" = 15)
    if (input.size() == 2 && input.mid(1) == tensUnit) {
        int prefix = digitMap.value(input.left(1), -1);
        if (prefix > 1) {
            return prefix * 10;
        }
        // "十" alone = 10
        if (prefix == -1 && input.left(1) == tensUnit) {
            return 10;
        }
    }

    // Three-character pattern: "X Y Z" where Y is tens unit (e.g., "二十五" = 25)
    if (input.size() == 3 && input.mid(1) == tensUnit) {
        int prefix = digitMap.value(input.left(1), -1);
        int suffix = digitMap.value(input.right(1), 0);
        if (prefix > 0) {
            return prefix * 10 + suffix;
        }
    }

    return -1;
}

QMap<QString, int> TimeExtractor::mapFromVariant(const QVariant &variant)
{
    QMap<QString, int> result;
    const QVariantMap map = variant.toMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        result.insert(it.key(), it.value().toInt());
    }
    return result;
}

QString TimeExtractor::name() const
{
    return QStringLiteral("time");
}

DFM_SEARCH_END_NS
