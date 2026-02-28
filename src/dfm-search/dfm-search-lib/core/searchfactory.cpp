// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/searchfactory.h>

DFM_SEARCH_BEGIN_NS

SearchEngine *SearchFactory::createEngine(SearchType type, QObject *parent)
{
    SearchEngine *engine = nullptr;

    switch (type) {
    case SearchType::FileName:
        engine = new SearchEngine(type, parent);
        break;
    case SearchType::Content:
        engine = new SearchEngine(type, parent);
        break;
    case SearchType::Custom:
        // TODO: 由应用程序基于provider自行创建
        break;
    }

    return engine;
}

SearchQuery SearchFactory::createQuery(const QString &keyword, SearchQuery::Type type)
{
    switch (type) {
    case SearchQuery::Type::Simple:
        return SearchQuery::createSimpleQuery(keyword);
    case SearchQuery::Type::Boolean:
        // NOTE: space
        return SearchQuery::createBooleanQuery(keyword.split(" "));
    case SearchQuery::Type::Wildcard:
        return SearchQuery(keyword, SearchQuery::Type::Wildcard);
    }

    return SearchQuery(keyword);
}

SearchQuery SearchFactory::createQuery(const QStringList &keywords, SearchQuery::Type type)
{
    if (type == SearchQuery::Type::Boolean)
        return SearchQuery::createBooleanQuery(keywords);

    return SearchQuery();
}

DFM_SEARCH_END_NS
