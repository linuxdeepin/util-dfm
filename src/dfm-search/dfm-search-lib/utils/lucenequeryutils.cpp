// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "lucenequeryutils.h"

#include <QDir>

DFM_SEARCH_BEGIN_NS

namespace LuceneQueryUtils {

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

}   // namespace LuceneQueryUtils

DFM_SEARCH_END_NS
