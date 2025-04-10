// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "searchutility.h"

#include <unistd.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/FSDirectory.h>

DFM_SEARCH_BEGIN_NS
using namespace Lucene;

// TODO (search): use dconfig
namespace Global {

static const QSet<QString> &supportedExtensions()
{
    static const QSet<QString> kExtensions = {
        "rtf", "odt", "ods", "odp", "odg", "docx",
        "xlsx", "pptx", "ppsx", "md", "xls", "xlsb",
        "doc", "dot", "wps", "ppt", "pps", "txt",
        "pdf", "dps", "sh", "html", "htm", "xml",
        "xhtml", "dhtml", "shtm", "shtml", "json",
        "css", "yaml", "ini", "bat", "js", "sql",
        "uof", "ofd"
    };
    return kExtensions;
}

bool isSupportedContentSearchExtension(const QString &suffix)
{
    return supportedExtensions().contains(suffix.toLower());
}

QStringList defaultContentSearchExtensions()
{
    return supportedExtensions().values();
}

QStringList defaultIndexedDirectory()
{
    return { QDir::homePath() };
}

bool isPathInContentIndexDirectory(const QString &path)
{
    if (!isContentIndexAvailable())
        return false;

    const QStringList &dirs = defaultIndexedDirectory();
    return std::any_of(dirs.cbegin(), dirs.cend(),
                       [&path](const QString &dir) { return path.startsWith(dir); });
}

bool isContentIndexAvailable()
{
    const QString &dir = contentIndexDirectory();
    if (!IndexReader::indexExists(FSDirectory::open(dir.toStdWString())))
        return false;

    const QString &statusFile = dir + "/index_status.json";

    // 1. 尝试打开文件
    QFile file(statusFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;   // 文件无法打开
    }

    // 2. 读取并解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        return false;   // JSON 格式无效
    }

    // 3. 检查 lastUpdateTime 字段
    QJsonObject obj = doc.object();
    if (!obj.contains("lastUpdateTime")) {
        return false;   // 字段不存在
    }

    const QString lastUpdateTime = obj["lastUpdateTime"].toString();
    return !lastUpdateTime.isEmpty();   // 字段值非空则为有效
}

QString contentIndexDirectory()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir deepinDir(configDir);
    QString indexPath = deepinDir.filePath("deepin/dde-file-manager/index");
    return indexPath;
}

bool isPathInFileNameIndexDirectory(const QString &path)
{
    if (!isFileNameIndexDirectoryAvailable())
        return false;

    const QStringList &dirs = defaultIndexedDirectory();
    return std::any_of(dirs.cbegin(), dirs.cend(),
                       [&path](const QString &dir) { return path.startsWith(dir); });
}

bool isFileNameIndexDirectoryAvailable()
{
    return IndexReader::indexExists(FSDirectory::open(fileNameIndexDirectory().toStdWString()));
}

QString fileNameIndexDirectory()
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
    static const QStringList kTypes { "app", "archive", "audio", "doc", "pic", "video", "dir", "other" };
    return kTypes;
}

bool isPurePinyin(const QString &str)
{
    static QRegularExpression regex(R"(^[a-zA-Z]+$)");
    return regex.match(str).hasMatch();
}

}   // namespace SearchUtility
DFM_SEARCH_END_NS
