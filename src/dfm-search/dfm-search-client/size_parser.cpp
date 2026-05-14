// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "size_parser.h"

namespace dfmsearch {

bool SizeParser::parseSize(const QString &arg, qint64 &bytes)
{
    if (arg.isEmpty()) {
        return false;
    }

    QString trimmed = arg.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    // 提取数字部分和单位后缀
    QString numStr;
    QString suffix;

    for (int i = 0; i < trimmed.length(); ++i) {
        QChar c = trimmed[i];
        if (c.isDigit() || c == '.') {
            numStr += c;
        } else {
            suffix = trimmed.mid(i).trimmed().toUpper();
            break;
        }
    }

    if (numStr.isEmpty()) {
        return false;
    }

    bool ok = false;
    double value = numStr.toDouble(&ok);
    if (!ok || value < 0) {
        return false;
    }

    // 根据后缀计算字节数
    qint64 multiplier = 1;
    if (suffix == "K" || suffix == "KB") {
        multiplier = 1024LL;
    } else if (suffix == "M" || suffix == "MB") {
        multiplier = 1024LL * 1024;
    } else if (suffix == "G" || suffix == "GB") {
        multiplier = 1024LL * 1024 * 1024;
    } else if (suffix == "T" || suffix == "TB") {
        multiplier = 1024LL * 1024 * 1024 * 1024;
    } else if (!suffix.isEmpty()) {
        // 未知后缀
        return false;
    }

    bytes = static_cast<qint64>(value * multiplier);
    return true;
}

}   // namespace dfmsearch
