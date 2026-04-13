// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIME_PARSER_H
#define TIME_PARSER_H

#include <QString>
#include <QDateTime>
#include <dfm-search/dsearch_global.h>

namespace dfmsearch {

/**
 * @brief 时间范围解析工具类
 *
 * 提供时间参数解析功能，遵循单一职责原则
 */
class TimeParser
{
public:
    /**
     * @brief 解析 --time-last 参数（如 "3d", "2h", "30m"）
     * @param arg 输入字符串
     * @param value 输出数值
     * @param unit 输出时间单位
     * @return 解析成功返回true
     */
    static bool parseTimeLast(const QString &arg, int &value, DFMSEARCH::TimeUnit &unit);

    /**
     * @brief 解析 --time-range 参数（如 "2025-01-01,2025-12-31"）
     * @param arg 输入字符串
     * @param start 输出开始时间
     * @param end 输出结束时间
     * @return 解析成功返回true
     */
    static bool parseTimeRange(const QString &arg, QDateTime &start, QDateTime &end);
};

}   // namespace dfmsearch

#endif   // TIME_PARSER_H
