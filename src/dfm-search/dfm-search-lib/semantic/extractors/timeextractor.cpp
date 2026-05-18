// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "timeextractor.h"

#include "semantic/semanticruleengine.h"

#include <QDate>
#include <QDateTime>
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
            { "today", TimePreset::Today },
            { "yesterday", TimePreset::Yesterday },
            { "day_before_yesterday", TimePreset::DayBeforeYesterday },
            { "this_week", TimePreset::ThisWeek },
            { "last_week", TimePreset::LastWeek },
            { "this_month", TimePreset::ThisMonth },
            { "last_month", TimePreset::LastMonth },
            { "this_year", TimePreset::ThisYear },
            { "last_year", TimePreset::LastYear },
        };

        if (kPresetMap.contains(presetStr)) {
            tc.kind = TimeConstraintKind::Preset;
            tc.preset = kPresetMap.value(presetStr);
        }
    } else if (typeStr == "custom") {
        parseCustomTime(match, metadata, tc);
    } else if (typeStr == "relative") {
        parseRelativeTime(metadata, tc);
    } else if (typeStr == "relative_dynamic") {
        parseDynamicRelativeTime(match, metadata, tc);
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

void TimeExtractor::parseRelativeTime(const QVariantMap &metadata, TimeConstraint &tc)
{
    const QDateTime now = QDateTime::currentDateTime();
    const int agoEndSecs = metadata.value("ago_end_seconds", 0).toInt();
    const int agoStartSecs = metadata.value("ago_start_seconds", 0).toInt();

    tc.kind = TimeConstraintKind::Relative;
    tc.customEnd = now.addSecs(-agoEndSecs);

    if (agoStartSecs < 0) {
        // Sentinel: "from epoch"
        tc.customStart = QDateTime::fromMSecsSinceEpoch(0);
    } else {
        tc.customStart = now.addSecs(-agoStartSecs);
    }
}

void TimeExtractor::parseDynamicRelativeTime(const QRegularExpressionMatch &match,
                                               const QVariantMap &metadata,
                                               TimeConstraint &tc)
{
    // Load locale-aware number conversion from rule metadata
    const QMap<QString, int> digitMap = mapFromVariant(metadata.value("digit_map"));
    const QString tensUnit = metadata.value("tens_unit").toString();

    // Extract numeric value from any of the named capture groups (value, value2, value3, value4)
    int value = 0;
    const QStringList captureNames = { QStringLiteral("value"), QStringLiteral("value2"),
                                        QStringLiteral("value3"), QStringLiteral("value4") };
    for (const QString &name : captureNames) {
        const QString captured = match.captured(name);
        if (!captured.isNull()) {
            value = localeAwareToInt(captured, digitMap, tensUnit);
            if (value > 0) {
                break;
            }
            value = 0;
        }
    }

    if (value <= 0 || value > 3650) {
        return;
    }

    // Determine unit from metadata or capture
    const QString unitStr = metadata.value("default_unit").toString();
    TimeUnit unit = TimeUnit::Days;
    if (unitStr == "hours") {
        unit = TimeUnit::Hours;
    } else if (unitStr == "weeks") {
        unit = TimeUnit::Weeks;
    } else if (unitStr == "months") {
        unit = TimeUnit::Months;
    }

    const QDateTime now = QDateTime::currentDateTime();
    qint64 totalSeconds = 0;

    switch (unit) {
    case TimeUnit::Minutes:
        totalSeconds = static_cast<qint64>(value) * 60;
        break;
    case TimeUnit::Hours:
        totalSeconds = static_cast<qint64>(value) * 3600;
        break;
    case TimeUnit::Days:
        totalSeconds = static_cast<qint64>(value) * 86400;
        break;
    case TimeUnit::Weeks:
        totalSeconds = static_cast<qint64>(value) * 7 * 86400;
        break;
    case TimeUnit::Months: {
        // Approximate: use average days per month
        const QDate startDate = now.addMonths(-value).date();
        tc.kind = TimeConstraintKind::Relative;
        tc.customStart = QDateTime(startDate, QTime(0, 0, 0));
        tc.customEnd = now;
        tc.relativeValue = value;
        tc.relativeUnit = unit;
        return;
    }
    case TimeUnit::Years:
        totalSeconds = static_cast<qint64>(value) * 365 * 86400;
        break;
    }

    tc.kind = TimeConstraintKind::Relative;
    tc.customStart = now.addSecs(-totalSeconds);
    tc.customEnd = now;
    tc.relativeValue = value;
    tc.relativeUnit = unit;
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
