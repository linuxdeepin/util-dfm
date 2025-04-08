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
            return QStringLiteral("(No content available)");
        }

        // 确定内容显示长度
        int contentLength = determineContentLength(content);
        if (contentLength <= 0) {
            contentLength = maxLength;
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

int determineContentLength(const QString &content)
{
    // 检查是否有换行符
    int newLinePos = content.indexOf('\n');
    if (newLinePos > 0) {
        // 如果有换行符，以换行符位置为界
        return newLinePos;
    }

    // 无换行符的文档(如PDF)，返回50
    return 50;
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

QString customHighlight(const QString &content, const Lucene::QueryPtr &query, int maxLength)
{
    // 从查询中提取关键词
    QStringList keywords = extractKeywords(query);
    if (keywords.isEmpty()) {
        // 无法提取关键词，返回内容前段
        int displayLength = qMin(maxLength, content.length());
        return content.left(displayLength) + (content.length() > displayLength ? "..." : "");
    }

    // 查找第一个关键词匹配
    int firstMatchPos = -1;
    QString matchedKeyword;

    for (const QString &keyword : keywords) {
        // 忽略大小写搜索
        int pos = content.toLower().indexOf(keyword.toLower());
        if (pos >= 0 && (firstMatchPos == -1 || pos < firstMatchPos)) {
            firstMatchPos = pos;
            matchedKeyword = keyword;
        }
    }

    if (firstMatchPos == -1) {
        // 没有找到匹配项，返回内容前段
        int displayLength = qMin(maxLength, content.length());
        return content.left(displayLength) + (content.length() > displayLength ? "..." : "");
    }

    return highlightKeywordInContext(content, matchedKeyword, firstMatchPos, maxLength);
}

QString highlightKeywordInContext(const QString &content, const QString &keyword,
                                  int matchPos, int maxLength)
{
    // 计算上下文范围
    int contextStart = qMax(0, matchPos - maxLength / 2);
    int keywordLength = keyword.length();
    int contextEnd = qMin(content.length(), matchPos + keywordLength + maxLength / 2);

    // 调整以避免截断单词
    if (contextStart > 0) {
        // 向前找到空格或标点
        int i = contextStart;
        while (i > 0 && !content[i].isSpace() && !content[i].isPunct()) {
            i--;
        }
        if (i > 0) contextStart = i + 1;
    }

    if (contextEnd < content.length()) {
        // 向后找到空格或标点
        int i = contextEnd;
        while (i < content.length() && !content[i].isSpace() && !content[i].isPunct()) {
            i++;
        }
        if (i < content.length()) contextEnd = i;
    }

    // 提取上下文并添加省略号
    QString result = content.mid(contextStart, contextEnd - contextStart);
    if (contextStart > 0) result = "..." + result;
    if (contextEnd < content.length()) result = result + "...";

    // 高亮关键词
    QString lcResult = result.toLower();
    QString lcKeyword = keyword.toLower();
    int pos = 0;

    while ((pos = lcResult.indexOf(lcKeyword, pos)) != -1) {
        // 检查是否需要整词匹配
        bool isWordStart = (pos == 0 || !lcResult[pos - 1].isLetterOrNumber());
        bool isWordEnd = (pos + lcKeyword.length() >= lcResult.length() || !lcResult[pos + lcKeyword.length()].isLetterOrNumber());

        // 对于英文单词，需要完整匹配，对于中文或特殊情况，可以松一些
        bool shouldHighlight = true;
        if (keyword.at(0).isLetter()) {
            // 英文单词需要整词匹配
            shouldHighlight = isWordStart && isWordEnd;
        }

        if (shouldHighlight) {
            // 获取原始大小写的关键词
            QString originalCaseKeyword = result.mid(pos, lcKeyword.length());
            // 替换为高亮版本
            result.replace(pos, lcKeyword.length(),
                           "<b style=\"color:red;\">" + originalCaseKeyword + "</b>");
            // 更新低版本结果和位置
            lcResult = result.toLower();
            pos += 29 + lcKeyword.length();
        } else {
            pos += lcKeyword.length();
        }
    }

    return result;
}

QStringList extractKeywords(const Lucene::QueryPtr &query)
{
    QStringList keywords;

    try {
        // 处理布尔查询
        BooleanQueryPtr boolQuery = boost::dynamic_pointer_cast<BooleanQuery>(query);
        if (boolQuery) {
            Collection<BooleanClausePtr> clauses = boolQuery->getClauses();
            for (int i = 0; i < clauses.size(); i++) {
                BooleanClausePtr clause = clauses[i];
                keywords.append(extractKeywords(clause->getQuery()));
            }
            return keywords;
        }

        // 处理词条查询
        TermQueryPtr termQuery = boost::dynamic_pointer_cast<TermQuery>(query);
        if (termQuery) {
            keywords.append(QString::fromStdWString(termQuery->getTerm()->text()));
            return keywords;
        }

        // 处理短语查询
        PhraseQueryPtr phraseQuery = boost::dynamic_pointer_cast<PhraseQuery>(query);
        if (phraseQuery) {
            Collection<TermPtr> terms = phraseQuery->getTerms();
            QStringList phraseWords;
            for (int i = 0; i < terms.size(); i++) {
                phraseWords.append(QString::fromStdWString(terms[i]->text()));
            }
            keywords.append(phraseWords.join(" "));
            return keywords;
        }

        // 处理通配符查询
        WildcardQueryPtr wildcardQuery = boost::dynamic_pointer_cast<WildcardQuery>(query);
        if (wildcardQuery) {
            // 使用去掉通配符的字符串
            QString termText = QString::fromStdWString(wildcardQuery->getTerm()->text());
            termText.remove('*').remove('?');
            if (!termText.isEmpty()) {
                keywords.append(termText);
            }
            return keywords;
        }

        // 如果是其他类型的查询，尝试获取查询文本
        String queryText = query->toString();
        QString qQueryText = QString::fromStdWString(queryText);

        // 手动解析查询文本
        qQueryText.remove(QRegularExpression(R"(\w+:)"));   // 移除字段名（如 "title:apple" → "apple"）
        qQueryText.remove(QRegularExpression(R"([$$\+\-\&\|\!\{\}$$$$\^\~\*\?\\:])"));   // 移除特殊字符
        qQueryText = qQueryText.trimmed();   // 去除前后空格

        if (!qQueryText.isEmpty()) {
            keywords.append(qQueryText);
        }
    } catch (const std::exception &e) {
        qWarning() << "Error extracting keywords from query:" << e.what();
    }

    return keywords;
}

}   // namespace ContentHighlighter

DFM_SEARCH_END_NS
