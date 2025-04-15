// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "contenthighlighter.h"

#include <QRegularExpression>
#include <QStringList>

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
    if (content.isEmpty() || keywords.isEmpty())
        return QString();

    KeywordMatch match = findFirstKeywordMatch(content, keywords);
    if (match.position == -1)
        return QString();

    // 如果关键词长度超过maxLength，直接返回关键词
    if (match.length >= maxLength) {
        return match.keyword;
    }

    int start = findOptimalStartPosition(content, match.position, maxLength);
    int end = findOptimalEndPosition(content, match.position, match.length, maxLength, start);

    QString result = content.mid(start, end - start).simplified();

    if (enableHtml) {
        for (const QString &keyword : keywords) {
            result = highlightKeyword(result, keyword);
        }
    }

    return result;
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
