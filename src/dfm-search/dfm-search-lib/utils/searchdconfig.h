// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHDCONFIG_H
#define SEARCHDCONFIG_H

#include <QStringList>
#include <QVariant>
#include <QMetaType>
#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

struct SearchDConfigSnapshot
{
    QStringList defaultIndexedDirs;
    QStringList defaultBlacklistPaths;
    QStringList supportedFileExtensions;
};

class SearchDConfig
{
public:
    static SearchDConfigSnapshot loadSnapshot();
    static QStringList loadFileExtensions();
    static QStringList loadBlacklistPaths();
    static QStringList loadIndexedDirs();
};

DFM_SEARCH_END_NS

Q_DECLARE_METATYPE(DFMSEARCH::SearchDConfigSnapshot)

#endif   // SEARCHDCONFIG_H
