// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSEARCH_GLOBAL_H
#define DSEARCH_GLOBAL_H

#include <QObject>

#define DFMSEARCH dfmsearch
#define DFM_SEARCH_BEGIN_NS namespace DFMSEARCH {
#define DFM_SEARCH_END_NS }
#define DFM_SEARCH_USE_NS using namespace DFMSEARCH;

DFM_SEARCH_BEGIN_NS
Q_NAMESPACE

// 搜索类型枚举
enum SearchType {
    FileName,   // 文件名搜索
    Content,   // 内容搜索
    Custom = 50   // 用户自定义搜索类型
};
Q_ENUM_NS(SearchType)

// 搜索状态枚举
enum SearchStatus {
    Ready,   // 准备就绪
    Searching,   // 正在搜索
    Finished,   // 已完成
    Cancelled,   // 已取消
    Error   // 错误

    // Pause ?
};
Q_ENUM_NS(SearchStatus)

// 搜索方式
enum SearchMethod {
    Indexed,   // 索引搜索
    Realtime   // 实时搜索
};
Q_ENUM_NS(SearchMethod)

DFM_SEARCH_END_NS

#endif   // DSEARCH_GLOBAL_H
