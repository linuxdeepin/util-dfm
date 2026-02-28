// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/searchquery.h>

DFM_SEARCH_BEGIN_NS

class SearchQueryData
{
public:
    SearchQueryData()
        : type(SearchQuery::Type::Simple),
          booleanOp(SearchQuery::BooleanOperator::AND)
    {
    }

    SearchQueryData(const QString &keyword, SearchQuery::Type type = SearchQuery::Type::Simple)
        : keyword(keyword),
          type(type),
          booleanOp(SearchQuery::BooleanOperator::AND)
    {
    }

    SearchQueryData(const SearchQueryData &other)
        : keyword(other.keyword),
          type(other.type),
          booleanOp(other.booleanOp),
          subQueries(other.subQueries)
    {
    }

    QString keyword;
    SearchQuery::Type type;
    SearchQuery::BooleanOperator booleanOp;
    QList<SearchQuery> subQueries;
};

SearchQuery::SearchQuery()
    : d(std::make_unique<SearchQueryData>())
{
}

SearchQuery::SearchQuery(const QString &keyword)
    : d(std::make_unique<SearchQueryData>(keyword))
{
}

SearchQuery::SearchQuery(const QString &keyword, Type type)
    : d(std::make_unique<SearchQueryData>(keyword, type))
{
}

SearchQuery::SearchQuery(const SearchQuery &other)
    : d(std::make_unique<SearchQueryData>(*other.d))
{
}

SearchQuery::SearchQuery(SearchQuery &&other) noexcept
    : d(std::move(other.d))
{
}

SearchQuery::~SearchQuery() = default;

SearchQuery &SearchQuery::operator=(const SearchQuery &other)
{
    if (this != &other) {
        d = std::make_unique<SearchQueryData>(*other.d);
    }
    return *this;
}

SearchQuery &SearchQuery::operator=(SearchQuery &&other) noexcept
{
    if (this != &other) {
        d = std::move(other.d);
    }
    return *this;
}

QString SearchQuery::keyword() const
{
    return d->keyword;
}

void SearchQuery::setKeyword(const QString &keyword)
{
    d->keyword = keyword;
}

SearchQuery::Type SearchQuery::type() const
{
    return d->type;
}

void SearchQuery::setType(Type type)
{
    d->type = type;
}

SearchQuery::BooleanOperator SearchQuery::booleanOperator() const
{
    return d->booleanOp;
}

void SearchQuery::setBooleanOperator(BooleanOperator op)
{
    d->booleanOp = op;
}

void SearchQuery::addSubQuery(const SearchQuery &query)
{
    d->subQueries.append(query);
}

QList<SearchQuery> SearchQuery::subQueries() const
{
    return d->subQueries;
}

void SearchQuery::clearSubQueries()
{
    d->subQueries.clear();
}

SearchQuery SearchQuery::createSimpleQuery(const QString &keyword)
{
    return SearchQuery(keyword, Type::Simple);
}

SearchQuery SearchQuery::createBooleanQuery(const QStringList &keywords, BooleanOperator op)
{
    SearchQuery query;
    query.setType(Type::Boolean);
    query.setBooleanOperator(op);

    for (const QString &keyword : keywords) {
        query.addSubQuery(SearchQuery(keyword, Type::Simple));
    }

    return query;
}

DFM_SEARCH_END_NS
