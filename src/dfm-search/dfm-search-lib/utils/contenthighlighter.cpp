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

}   // namespace ContentHighlighter

DFM_SEARCH_END_NS
