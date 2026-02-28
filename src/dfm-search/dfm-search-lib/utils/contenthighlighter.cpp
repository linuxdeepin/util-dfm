// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "contenthighlighter.h"

#include <QRegularExpression>
#include <QStringList>
#include <QDebug>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/Highlighter.h>
#include <lucene++/SimpleHTMLFormatter.h>
#include <lucene++/SimpleFragmenter.h>
#include <lucene++/QueryScorer.h>
#include <lucene++/StringReader.h>

#include "chineseanalyzer.h"

using namespace Lucene;

DFM_SEARCH_BEGIN_NS

namespace ContentHighlighter {

namespace {
QString mergeAdjacentHighlightTags(const QString &text)
{
    // 使用正则表达式搜索和替换相邻的高亮标签
    QString result = text;

    // 替换模式：</b><b style="color:red;"> 将被删除，从而合并相邻的标签
    static const QString pattern = QLatin1String("</b><b>");
    static const QString replacement = QLatin1String("");

    // 循环替换直到不再有变化（处理连续多个标签的情况）
    QString previousResult;
    do {
        previousResult = result;
        result = result.replace(pattern, replacement);
    } while (result != previousResult);

    return result;
}
}   // namespace

namespace {

struct KeywordMatch
{
    int position;
    int length;
    QString keyword;
};

/**
 * @brief 检查给定位置是否是段落的开头
 * @param content 原始文本内容
 * @param position 需要检查的位置
 * @return 如果是段落开头返回true，否则返回false
 */
bool isParagraphStart(const QString &content, int position)
{
    // 如果位置为0，则一定是文档开头
    if (position == 0) {
        return true;
    }

    // 如果位置超出内容范围，返回false
    if (position >= content.length()) {
        return false;
    }

    // 向前搜索，跳过所有空白字符
    int lookBackPos = position - 1;
    while (lookBackPos >= 0 && content.at(lookBackPos).isSpace() && content.at(lookBackPos) != '\n' && content.at(lookBackPos) != '\r') {
        lookBackPos--;
    }

    // 如果找到段落结束符或已到文档开头，则认为是段落开头
    if (lookBackPos < 0) {
        return true;   // 文档开头
    }

    QChar prevChar = content.at(lookBackPos);
    return (prevChar == '\n' || prevChar == '\r');
}

int findOptimalStartPosition(const QString &content, int keywordPos, int maxLength)
{
    if (keywordPos == 0) return 0;

    // 计算关键词前后的理想长度
    int idealBeforeLength = maxLength / 2;
    int idealAfterLength = maxLength - idealBeforeLength;

    // 如果关键词在文档开头附近，直接返回0
    if (keywordPos <= idealBeforeLength) {
        return 0;
    }

    // 如果关键词在文档末尾附近，确保有足够的空间显示关键词前的文本
    if (keywordPos + idealAfterLength >= content.length()) {
        return qMax(0, content.length() - maxLength);
    }

    // 尝试找到最近的换行符作为起始点
    int start = keywordPos - idealBeforeLength;
    int lastNewline = content.lastIndexOf('\n', keywordPos);

    // 如果找到换行符，并且它在理想起始点之后，则使用换行符作为起始点
    if (lastNewline != -1 && lastNewline > start) {
        start = lastNewline + 1;
    }

    // 确保起始位置不会导致文本超出maxLength
    if (keywordPos - start + idealAfterLength > maxLength) {
        start = keywordPos - (maxLength - idealAfterLength);
    }

    return qMax(0, start);
}

int findOptimalEndPosition(const QString &content, int keywordPos, int keywordLength, int maxLength, int startPos)
{
    // 计算关键词前后的理想长度
    int idealBeforeLength = keywordPos - startPos;
    int idealAfterLength = maxLength - idealBeforeLength;

    // 如果关键词在文档末尾，直接返回文档末尾
    if (keywordPos + keywordLength >= content.length()) {
        return content.length();
    }

    // 尝试找到下一个换行符作为结束点
    int end = keywordPos + keywordLength + idealAfterLength;
    int nextNewline = content.indexOf('\n', keywordPos + keywordLength);

    // 如果找到换行符，并且它在理想结束点之前，则使用换行符作为结束点
    if (nextNewline != -1 && nextNewline < end) {
        end = nextNewline;
    }

    // 确保结束位置不会导致文本超出maxLength
    if (end - startPos > maxLength) {
        end = startPos + maxLength;
    }

    return qMin(content.length(), end);
}

QString highlightKeyword(const QString &text, const QString &keyword)
{
    if (text.isEmpty() || keyword.isEmpty()) return text;
    QString result = text;
    int pos = 0;
    const int keywordLength = keyword.length();
    while ((pos = result.indexOf(keyword, pos, Qt::CaseInsensitive)) != -1) {
        result.insert(pos + keywordLength, "</b>");
        result.insert(pos, "<b>");
        pos += keywordLength + 7;
    }
    return result;
}

KeywordMatch findFirstKeywordMatch(const QString &content, const QStringList &keywords)
{
    KeywordMatch match = { -1, 0, QString() };
    for (const QString &keyword : keywords) {
        int pos = content.indexOf(keyword, 0, Qt::CaseInsensitive);
        if (pos != -1 && (match.position == -1 || pos < match.position)) {
            match.position = pos;
            match.length = keyword.length();
            match.keyword = keyword;
            break;
        }
    }
    return match;
}
}   // namespace

QString customHighlight(const QStringList &keywords, const QString &content, int maxLength, bool enableHtml)
{
    if (content.isEmpty() || keywords.isEmpty()) {
        return QString();
    }

    // Ensure there's at least one non-empty keyword.
    // findFirstKeywordMatch handles empty strings in the list, but if all are empty, it's like no keywords.
    bool hasValidKeyword = false;
    for (const QString &kw : keywords) {
        if (!kw.isEmpty()) {
            hasValidKeyword = true;
            break;
        }
    }
    if (!hasValidKeyword) {
        return QString();
    }

    KeywordMatch match = findFirstKeywordMatch(content, keywords);
    if (match.position == -1) {
        // No keyword found in content.
        // As per problem, if no keyword, no range to show.
        // Alternative: return content.left(maxLength).simplified(); if some default text is needed.
        return QString();
    }

    // If the keyword itself is longer than or equal to the final desired maxLength
    if (match.length >= maxLength) {
        if (enableHtml) {
            // Highlight the keyword itself if HTML is enabled
            return highlightKeyword(match.keyword, match.keyword);
        }
        return match.keyword;   // Return the keyword as is (original behavior)
    }

    // This is the "30 characters" from the requirement, used for positioning the keyword.
    const int positioningMaxLength = 30;

    // 1. Calculate the optimal start position.
    //    This start position is determined based on making the keyword visible
    //    and well-positioned within a `positioningMaxLength` (e.g., 80 char) window.
    int optimalStart = findOptimalStartPosition(content, match.position, positioningMaxLength);

    // 2. Calculate the optimal end position.
    //    This uses the `optimalStart` calculated above and extends the snippet
    //    up to the overall `maxLength` (e.g., 200 chars for the final result),
    //    or the end of the content, whichever is first.
    int optimalEnd = findOptimalEndPosition(content, match.position, match.length, maxLength, optimalStart);

    // Safeguard: If somehow optimalStart >= optimalEnd, try to at least show the keyword.
    // This can happen if content is very short or keyword is at the very end and newline logic truncates aggressively.
    if (optimalStart >= optimalEnd && match.length > 0) {
        optimalStart = match.position;
        optimalEnd = qMin(content.length(), match.position + match.length);
        // If still bad, it means something is wrong, but mid(X,0) or mid(X, negative) is empty string
    }

    QString resultSnippet = content.mid(optimalStart, optimalEnd - optimalStart);

    // .simplified() removes leading/trailing whitespace and replaces multiple internal whitespaces with one.
    // This is applied *after* extraction.
    resultSnippet = resultSnippet.simplified();

    if (enableHtml) {
        // Highlight all keywords from the list that appear in the *extracted and simplified* snippet.
        // Create a temporary string for highlighting because highlightKeyword modifies the string it's given,
        // and we are iterating.
        QString tempHighlightedSnippet = resultSnippet;
        for (const QString &kwToHighlight : keywords) {
            if (!kwToHighlight.isEmpty()) {
                tempHighlightedSnippet = highlightKeyword(tempHighlightedSnippet, kwToHighlight);
            }
        }
        resultSnippet = tempHighlightedSnippet;
    }

    // 只有在段落开头被截断时才添加省略号
    if (optimalStart > 0 && !isParagraphStart(content, optimalStart)) {
        // Unicode 水平省略号字符 (U+2026)
        static constexpr QChar kEllipsis(8230);
        resultSnippet = QString(kEllipsis) + resultSnippet;
    }

    return resultSnippet;
}

QString highlight(const QString &content, const Lucene::QueryPtr &query, int maxLength, bool enableHtml)
{
    try {
        if (content.isEmpty()) {
            return {};
        }

        // 尝试使用Lucene高亮器
        FormatterPtr formatter;
        if (enableHtml) {
            formatter = newLucene<SimpleHTMLFormatter>(L"<b>", L"</b>");
        } else {
            formatter = newLucene<SimpleHTMLFormatter>(L"", L"");
        }
        HighlighterScorerPtr scorer = newLucene<QueryScorer>(query);
        HighlighterPtr highlighter = newLucene<Highlighter>(formatter, scorer);

        // 创建分析器
        AnalyzerPtr analyzer = newLucene<ChineseAnalyzer>();

        TokenStreamPtr tokenStream = analyzer->tokenStream(L"contents", newLucene<StringReader>(content.toStdWString()));
        Collection<String> fragments = highlighter->getBestFragments(tokenStream, content.toStdWString(), 1);

        QString result;
        if (!fragments.empty() && !fragments[0].empty()) {
            // Lucene高亮成功，使用其结果
            result = QString::fromStdWString(fragments[0]);
        } else {
            // TODO: Lucene高亮失败，使用自定义高亮方法
            // result = customHighlight(content, query, contentLength);
        }

        // 处理连续的高亮标签
        if (enableHtml) {
            result = mergeAdjacentHighlightTags(result);
        }

        return result.simplified();
    } catch (const LuceneException &e) {
        qWarning() << "Highlighting failed:" << QString::fromStdWString(e.getError());
        return QStringLiteral("(Error highlighting content)");
    }
}

}   // namespace ContentHighlighter

DFM_SEARCH_END_NS
