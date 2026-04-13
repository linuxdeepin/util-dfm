// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "time_parser.h"

#include <QRegularExpression>
#include <QStringList>

using namespace dfmsearch;

bool TimeParser::parseTimeLast(const QString &arg, int &value, DFMSEARCH::TimeUnit &unit)
{
    if (arg.isEmpty())
        return false;

    // 提取数字和单位
    QRegularExpression re("^(\\d+)([mhdwMy])$");
    QRegularExpressionMatch match = re.match(arg);

    if (!match.hasMatch())
        return false;

    bool ok;
    value = match.captured(1).toInt(&ok);
    if (!ok || value < 0)
        return false;

    QString unitChar = match.captured(2);
    if (unitChar == "m") {
        unit = DFMSEARCH::TimeUnit::Minutes;
    } else if (unitChar == "h") {
        unit = DFMSEARCH::TimeUnit::Hours;
    } else if (unitChar == "d") {
        unit = DFMSEARCH::TimeUnit::Days;
    } else if (unitChar == "w") {
        unit = DFMSEARCH::TimeUnit::Weeks;
    } else if (unitChar == "M") {
        unit = DFMSEARCH::TimeUnit::Months;
    } else if (unitChar == "y") {
        unit = DFMSEARCH::TimeUnit::Years;
    } else {
        return false;
    }

    return true;
}

bool TimeParser::parseTimeRange(const QString &arg, QDateTime &start, QDateTime &end)
{
    if (arg.isEmpty())
        return false;

    QStringList parts = arg.split(',');
    if (parts.size() != 2)
        return false;

    QString startStr = parts[0].trimmed();
    QString endStr = parts[1].trimmed();

    // 优先尝试带时间的格式，再尝试不带时间的
    start = QDateTime::fromString(startStr, "yyyy-MM-dd HH:mm");
    if (!start.isValid()) {
        start = QDateTime::fromString(startStr, "yyyy-MM-dd");
        if (start.isValid()) {
            start.setTime(QTime(0, 0, 0));
        }
    }

    end = QDateTime::fromString(endStr, "yyyy-MM-dd HH:mm");
    if (!end.isValid()) {
        end = QDateTime::fromString(endStr, "yyyy-MM-dd");
        if (end.isValid()) {
            end.setTime(QTime(23, 59, 59));
        }
    }

    return start.isValid() && end.isValid();
}
