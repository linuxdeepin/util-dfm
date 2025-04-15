#ifndef CONTENTHIGHLIGHTER_H
#define CONTENTHIGHLIGHTER_H

#include <QString>
#include <QStringList>

// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <lucene++/LuceneHeaders.h>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

namespace ContentHighlighter {

/**
 * @brief 高亮搜索结果中的关键词
 * @param content 要高亮的内容
 * @param query Lucene查询对象
 * @param maxLength 最大显示长度
 * @param enableHtml 是否启用HTML标签高亮，默认为false
 * @return 高亮后的内容
 */
QString highlight(const QString &content, const Lucene::QueryPtr &query, int maxLength = 50, bool enableHtml = false);

/**
 * @brief 合并相邻的高亮标签
 * @param text 包含高亮标签的文本
 * @return 处理后的文本
 */
QString mergeAdjacentHighlightTags(const QString &text);

}   // namespace ContentHighlighter

DFM_SEARCH_END_NS

#endif   // CONTENTHIGHLIGHTER_H
