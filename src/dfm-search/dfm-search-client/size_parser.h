// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIZE_PARSER_H
#define SIZE_PARSER_H

#include <QString>

namespace dfmsearch {

/**
 * @brief 文件大小参数解析工具类
 *
 * 支持解析人类可读的文件大小字符串，如 "1K", "10M", "1G", "512"
 * 不带后缀的纯数字视为字节数。
 */
class SizeParser
{
public:
    /**
     * @brief 解析文件大小字符串
     * @param arg 输入字符串（如 "1K", "10M", "1G", "512"）
     * @param bytes 输出字节数
     * @return 解析成功返回true
     *
     * 支持的后缀（不区分大小写）：
     * - K/KB: 千字节 (1024)
     * - M/MB: 兆字节 (1024^2)
     * - G/GB: 吉字节 (1024^3)
     * - T/TB: 太字节 (1024^4)
     * - 无后缀: 纯字节数
     */
    static bool parseSize(const QString &arg, qint64 &bytes);
};

}   // namespace dfmsearch

#endif   // SIZE_PARSER_H
