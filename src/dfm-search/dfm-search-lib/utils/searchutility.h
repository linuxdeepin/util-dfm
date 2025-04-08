// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHUTILITY_H
#define SEARCHUTILITY_H

#include <QStringList>
#include <QString>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchquery.h>

DFM_SEARCH_BEGIN_NS

namespace SearchUtility {

/**
 * @brief 获取索引目录路径
 * @return 返回系统索引目录路径
 */
QString anythingIndexDirectory();

/**
 * @brief 从查询中提取关键词列表
 * @param query 搜索查询
 * @return 关键词列表
 */
QStringList extractBooleanKeywords(const SearchQuery &query);

QStringList deepinAnythingFileTypes();

bool isPurePinyin(const QString &str);

}   // namespace SearchUtility
DFM_SEARCH_END_NS

#endif   // SEARCHUTILITY_H
