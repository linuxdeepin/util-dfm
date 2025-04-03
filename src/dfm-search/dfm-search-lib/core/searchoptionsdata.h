// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHOPTIONSDATA_H
#define SEARCHOPTIONSDATA_H

#include <QVariantMap>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief SearchOptions的私有实现类
 */
class SearchOptionsData
{
public:
    SearchOptionsData();
    SearchOptionsData(const SearchOptionsData &other);

    // 公共数据字段
    SearchMethod method;
    bool caseSensitive;
    QString searchPath;
    QStringList excludePaths;
    bool includeHidden;
    int maxResults;
    QVariantMap customOptions;
    bool recursive = true;   // 默认递归搜索
};

DFM_SEARCH_END_NS

#endif   // SEARCHOPTIONSDATA_H
