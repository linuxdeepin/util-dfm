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
 * @param maxLength   The maximum length of the returned snippet (used when no line breaks are present).
 * @param enableHtml  Whether to wrap matched keywords with <b></b> tags for highlighting.
 * @return A snippet of content with matched keywords highlighted, or an empty string if no match is found.
 */
QString customHighlight(const QStringList &keywords, const QString &content, int maxLength, bool enableHtml);

/**
 * @brief 高亮搜索结果中的关键词
 * @param content 要高亮的内容
 * @param query Lucene查询对象
 * @param maxLength 最大显示长度
 * @param enableHtml 是否启用HTML标签高亮，默认为false
 * @return 高亮后的内容
 */
QString highlight(const QString &content, const Lucene::QueryPtr &query, int maxLength, bool enableHtml);

}   // namespace ContentHighlighter

DFM_SEARCH_END_NS

#endif   // CONTENTHIGHLIGHTER_H
