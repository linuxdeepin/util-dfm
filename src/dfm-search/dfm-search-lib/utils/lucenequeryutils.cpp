// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "lucenequeryutils.h"

#include <QDir>

#include <lucene++/BooleanQuery.h>
#include <lucene++/PhraseQuery.h>
#include <lucene++/TermQuery.h>

DFM_SEARCH_BEGIN_NS

namespace LuceneQueryUtils {
namespace {

Lucene::String toLuceneString(const QString &str, bool caseSensitive)
{
    QString normalized = caseSensitive ? str : str.toLower();
    QByteArray utf8Bytes = normalized.toUtf8();
    Lucene::String luceneStr = Lucene::StringUtils::toUnicode(std::string(utf8Bytes.constData(), utf8Bytes.length()));
    if (luceneStr.empty()) {
        luceneStr = Lucene::StringUtils::toUnicode(normalized.toStdString());
    }
    return luceneStr;
}

Lucene::TermPtr buildTerm(const QString &fieldName, const QString &text, bool caseSensitive)
{
    return Lucene::newLucene<Lucene::Term>(
            toLuceneString(fieldName, true),
            toLuceneString(text, caseSensitive));
}

int phrasePositionForStandardNGram2(int startOffset)
{
    // Standard lucene++ NGramTokenizer(1,2) emits 1-gram then 2-gram at each
    // character offset, and every emitted token advances the phrase position by 1.
    // Therefore the 2-gram starting at offset i lands at position 2 * i + 1.
    return startOffset * 2 + 1;
}

}   // namespace

std::wstring getLuceneSpecialChars()
{
    // Lucene 特殊字符（按 Lucene 语法需要转义）
    return L"+-&&||!(){}[]^\"~*?:\\/ ";
}

Lucene::String processQueryString(const QString &str, bool caseSensitive)
{
    // Step 1: 转成 std::wstring
    std::wstring input = str.toStdWString();

    // Step 2: 转义 Lucene 特殊字符
    const std::wstring specialChars = getLuceneSpecialChars();

    std::wstring escaped;
    escaped.reserve(input.size() * 2);   // 预留空间，提高性能

    for (wchar_t ch : input) {
        // 如果是 Lucene 特殊字符，就在前面加 '\'
        if (specialChars.find(ch) != std::wstring::npos) {
            escaped += L'\\';
        }
        escaped += ch;
    }

    // Step 3: 转成 Lucene::String（Lucene++ 使用的是 UTF-8）
    QString tempQString = QString::fromStdWString(escaped);
    QByteArray utf8Bytes = tempQString.toUtf8();
    Lucene::String luceneStr = Lucene::StringUtils::toUnicode(std::string(utf8Bytes.constData(), utf8Bytes.length()));
    if (luceneStr.empty()) {
        luceneStr = Lucene::StringUtils::toUnicode(str.toStdString());
    }

    // Step 4: 如果不区分大小写就转小写（Lucene::String 是 Unicode）
    if (!caseSensitive) {
        Lucene::StringUtils::toLower(luceneStr);
    }

    return luceneStr;
}

Lucene::QueryPtr buildNGramSearchQuery(const QString &fieldName, const QString &keyword, bool caseSensitive)
{
    if (fieldName.isEmpty() || keyword.isEmpty()) {
        return nullptr;
    }

    if (keyword.size() <= 2) {
        return Lucene::newLucene<Lucene::TermQuery>(
                buildTerm(fieldName, keyword, caseSensitive));
    }

    Lucene::PhraseQueryPtr phraseQuery = Lucene::newLucene<Lucene::PhraseQuery>();
    for (int pos = 0; pos + 2 <= keyword.size(); pos += 2) {
        phraseQuery->add(buildTerm(fieldName, keyword.mid(pos, 2), caseSensitive),
                         phrasePositionForStandardNGram2(pos));
    }

    if (keyword.size() % 2 != 0) {
        const int tailPos = keyword.size() - 2;
        phraseQuery->add(buildTerm(fieldName, keyword.mid(tailPos, 2), caseSensitive),
                         phrasePositionForStandardNGram2(tailPos));
    }

    return phraseQuery;
}

Lucene::QueryPtr buildPathPrefixQuery(const QString &pathPrefix, const QString &fieldName)
{
    if (pathPrefix.isEmpty() || fieldName.isEmpty()) {
        return nullptr;
    }

    // 标准化路径：去掉多余的斜杠，去掉末尾的斜杠 (例如 "/home/user/" -> "/home/user")
    const QString normalizedPath = QDir::cleanPath(pathPrefix);

    return Lucene::newLucene<Lucene::TermQuery>(
            Lucene::newLucene<Lucene::Term>(
                    Lucene::StringUtils::toUnicode(fieldName.toStdString()),
                    Lucene::StringUtils::toUnicode(normalizedPath.toStdString())));
}

Lucene::QueryPtr buildMultiPathPrefixQuery(const QStringList &paths, const QString &fieldName)
{
    if (paths.isEmpty() || fieldName.isEmpty()) {
        return nullptr;
    }

    if (paths.size() == 1) {
        return buildPathPrefixQuery(paths.first(), fieldName);
    }

    Lucene::BooleanQueryPtr boolQuery = Lucene::newLucene<Lucene::BooleanQuery>();
    bool hasValid = false;

    for (const QString &path : paths) {
        Lucene::QueryPtr pathQuery = buildPathPrefixQuery(path, fieldName);
        if (pathQuery) {
            boolQuery->add(pathQuery, Lucene::BooleanClause::SHOULD);
            hasValid = true;
        }
    }

    return hasValid ? boolQuery : nullptr;
}

}   // namespace LuceneQueryUtils

DFM_SEARCH_END_NS
