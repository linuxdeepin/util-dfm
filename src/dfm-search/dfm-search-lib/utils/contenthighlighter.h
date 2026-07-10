// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONTENTHIGHLIGHTER_H
#define CONTENTHIGHLIGHTER_H

#include <QString>
#include <QStringList>

#include <lucene++/LuceneHeaders.h>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

namespace ContentHighlighter {

/**
 * @brief Extracts and highlights the most relevant text snippet from content based on the provided keywords.
 *
 * This function searches for the first occurrence(s) of the specified keywords within the input content and returns
 * a concise, contextually meaningful snippet containing the match(es). The snippet is intelligently trimmed based
 * on paragraph structure (if present) or character length, and keywords are optionally wrapped with <b> tags for
 * highlighting.
 *
 * Behavior Details:
 * - Case-insensitive matching is used.
 * - Wildcards '*' and '?' are supported in keywords.
 * - If content contains line breaks, the entire matching line is returned.
 * - Otherwise, a substring centered around the first match is returned, constrained by maxLength.
 * - If multiple keywords are given, only the first matching fragment is returned, with as many highlights as possible within the limit.
 * - No match results in an empty string.
 *
 * @param keywords    A list of search keywords (supports wildcards *, ?).
 * @param content     The original document content to be searched.
 * @param maxLength           The maximum length of the returned snippet (used when no line breaks are present).
 * @param enableHtml          Whether to wrap matched keywords with <b></b> tags for highlighting.
 * @param positioningMaxLength Keyword positioning window size for finding the optimal snippet start/end (min 30).
 * @return A snippet of content with matched keywords highlighted, or an empty string if no match is found.
 */
QString customHighlight(const QStringList &keywords, const QString &content, int maxLength, bool enableHtml, int positioningMaxLength = 30);

/**
 * @brief 精确预览：从 offset 位置截取内容，或从 offset 搜索 keyword 后从匹配位置截取
 *
 * 无 keyword: content.mid(offset, maxLength)
 * 有 keyword: content.indexOf(keyword, offset) → 从匹配位置截取 maxLength
 * 不做 simplified，不加省略号，不高亮
 *
 * maxLength <= 0 表示无限制，返回从 offset（或 keyword 匹配位置）开始的全部剩余内容。
 *
 * @param content 原始文档内容
 * @param offset  内容偏移值（默认0）
 * @param maxLength 最大截取长度，<= 0 表示无限制
 * @param keyword  搜索关键词（可选）
 * @param keywordOffset 输出 keyword 在全文中的匹配位置（-1 if 无 keyword 或未匹配）
 * @return 截取的内容片段，找不到 keyword 或 offset 越界时返回空字符串
 */
QString previewSnippet(const QString &content, int offset, int maxLength,
                       const QString &keyword = {},
                       int *keywordOffset = nullptr);

}   // namespace ContentHighlighter

DFM_SEARCH_END_NS

#endif   // CONTENTHIGHLIGHTER_H
