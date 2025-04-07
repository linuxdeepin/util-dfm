// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "searchutility.h"

#include <QDir>
#include <QFileInfo>
#include <unistd.h>

DFM_SEARCH_BEGIN_NS
namespace SearchUtility {

QString getHomeDirectory()
{
    QString homeDir;

    if (QFileInfo::exists("/data/home")) {
        homeDir = "/data";
    } else if (QFileInfo::exists("/persistent/home")) {
        homeDir = "/persistent";
    }

    homeDir.append(QDir::homePath());

    return homeDir;
}

QString getAnythingIndexDirectory()
{
    return QString("/run/user/%1/deepin-anything-server").arg(getuid());
}

QStringList extractKeywords(const SearchQuery &query)
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

}   // namespace SearchUtility
DFM_SEARCH_END_NS
