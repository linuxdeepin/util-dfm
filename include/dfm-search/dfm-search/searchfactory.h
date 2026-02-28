// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHFACTORY_H
#define SEARCHFACTORY_H

#include <dfm-search/searchengine.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>
#include <dfm-search/searchquery.h>

DFM_SEARCH_BEGIN_NS

class SearchFactory
{
public:
    static SearchEngine *createEngine(SearchType type, QObject *parent = nullptr);
    static SearchQuery createQuery(const QString &keyword, SearchQuery::Type type = SearchQuery::Type::Simple);
    static SearchQuery createQuery(const QStringList &keywords, SearchQuery::Type type = SearchQuery::Type::Boolean);
};

DFM_SEARCH_END_NS

#endif   // SEARCHFACTORY_H
