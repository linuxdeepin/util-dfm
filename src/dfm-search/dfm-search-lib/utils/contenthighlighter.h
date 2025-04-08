#ifndef CONTENTHIGHLIGHTER_H
#define CONTENTHIGHLIGHTER_H

#include <QString>
#include <QStringList>

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
 * @brief 确定内容显示长度
 * @param content 内容
 * @return 建议的显示长度
 */
int determineContentLength(const QString &content);

/**
 * @brief 合并相邻的高亮标签
 * @param text 包含高亮标签的文本
 * @return 处理后的文本
 */
QString mergeAdjacentHighlightTags(const QString &text);

/**
 * @brief 自定义高亮实现
 * @param content 要高亮的内容
 * @param query Lucene查询对象
 * @param maxLength 最大显示长度
 * @return 高亮后的内容
 */
QString customHighlight(const QString &content, const Lucene::QueryPtr &query, int maxLength);

/**
 * @brief 在上下文中高亮关键词
 * @param content 要高亮的内容
 * @param keyword 关键词
 * @param matchPos 匹配位置
 * @param maxLength 最大显示长度
 * @return 高亮后的内容
 */
QString highlightKeywordInContext(const QString &content, const QString &keyword, 
                                int matchPos, int maxLength);

/**
 * @brief 从查询中提取关键词
 * @param query Lucene查询对象
 * @return 关键词列表
 */
QStringList extractKeywords(const Lucene::QueryPtr &query);

}   // namespace ContentHighlighter

DFM_SEARCH_END_NS

#endif   // CONTENTHIGHLIGHTER_H 