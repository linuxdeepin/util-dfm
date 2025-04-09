// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "searchutility.h"

#include <unistd.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

DFM_SEARCH_BEGIN_NS

// TODO (search): use dconfig
namespace Global {

static const QSet<QString> &supportedExtensions()
{
    static const QSet<QString> extensions = {
        "rtf", "odt", "ods", "odp", "odg", "docx",
        "xlsx", "pptx", "ppsx", "md", "xls", "xlsb",
        "doc", "dot", "wps", "ppt", "pps", "txt",
        "pdf", "dps", "sh", "html", "htm", "xml",
        "xhtml", "dhtml", "shtm", "shtml", "json",
        "css", "yaml", "ini", "bat", "js", "sql",
        "uof", "ofd"
    };
    return extensions;
}

bool isSupportedFullTextSearchExtension(const QString &suffix)
{
    return supportedExtensions().contains(suffix.toLower());
}

QStringList defaultFullTextSearchExtensions()
{
    return supportedExtensions().values();
}

QString defaultIndexedDirectory()
{
    return QDir::homePath();
}

QString contentIndexDirectory()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir deepinDir(configDir);
    QString indexPath = deepinDir.filePath("deepin/dde-file-manager/index");
    return indexPath;
}

QString anythingIndexDirectory()
{
    return QString("/run/user/%1/deepin-anything-server").arg(getuid());
}

}   //  namespace Global

namespace SearchUtility {

QStringList extractBooleanKeywords(const SearchQuery &query)
{
    QStringList keywords;

    if (query.type() == SearchQuery::Type::Boolean) {
        // 布尔查询，提取所有子查询的关键词
        for (const auto &subQuery : query.subQueries()) {
            keywords.append(subQuery.keyword());
        }
        // 如果没有子查询，添加主关键词
        if (keywords.isEmpty()) {
            keywords.append(query.keyword());
        }
    } else {
        // 简单查询，直接添加关键词
        keywords.append(query.keyword());
    }

    // 移除空关键词
    keywords.removeAll("");

    return keywords;
}

QStringList deepinAnythingFileTypes()
{
    static const QStringList kTypes { "app", "archive", "audio", "doc", "pic", "video" };
    return kTypes;
}

bool isPurePinyin(const QString &str)
{
    static QRegularExpression regex(R"(^[a-zA-Z]+$)");
    return regex.match(str).hasMatch();
}

}   // namespace SearchUtility
DFM_SEARCH_END_NS
