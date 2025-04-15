// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/searchoptions.h>

#include <QDir>

#include "searchoptionsdata.h"

DFM_SEARCH_BEGIN_NS

SearchOptionsData::SearchOptionsData()
    : method(SearchMethod::Indexed),
      caseSensitive(false),
      searchPath(QDir::homePath()),
      includeHidden(false),
      maxResults(-1),
      enableResultFound(false)
{
}

/////////////

SearchOptions::SearchOptions()
    : d(std::make_unique<SearchOptionsData>())
{
}

SearchOptions::SearchOptions(const SearchOptions &other)
    : d(std::make_unique<SearchOptionsData>(*other.d))
{
}

SearchOptions::SearchOptions(SearchOptions &&other) noexcept
    : d(std::move(other.d))
{
}

SearchOptions::~SearchOptions() = default;

SearchOptions &SearchOptions::operator=(const SearchOptions &other)
{
    if (this != &other) {
        d = std::make_unique<SearchOptionsData>(*other.d);
    }
    return *this;
}

SearchOptions &SearchOptions::operator=(SearchOptions &&other) noexcept
{
    if (this != &other) {
        d = std::move(other.d);
    }
    return *this;
}

SearchMethod SearchOptions::method() const
{
    return d->method;
}

void SearchOptions::setSearchMethod(SearchMethod method)
{
    d->method = method;
}

bool SearchOptions::caseSensitive() const
{
    return d->caseSensitive;
}

void SearchOptions::setCaseSensitive(bool sensitive)
{
    d->caseSensitive = sensitive;
}

QString SearchOptions::searchPath() const
{
    return d->searchPath;
}

void SearchOptions::setSearchPath(const QString &path)
{
    d->searchPath = path;
}

QStringList SearchOptions::searchExcludedPaths() const
{
    return d->searchExcludedPaths;
}

void SearchOptions::setSearchExcludedPaths(const QStringList &excludedPaths)
{
    d->searchExcludedPaths = excludedPaths;
}

void SearchOptions::setIncludeHidden(bool include)
{
    d->includeHidden = include;
}

bool SearchOptions::includeHidden() const
{
    return d->includeHidden;
}

int SearchOptions::maxResults() const
{
    return d->maxResults;
}

void SearchOptions::setMaxResults(int count)
{
    d->maxResults = count;
}

void SearchOptions::setCustomOption(const QString &key, const QVariant &value)
{
    d->customOptions[key] = value;
}

QVariant SearchOptions::customOption(const QString &key) const
{
    return d->customOptions.value(key);
}

bool SearchOptions::hasCustomOption(const QString &key) const
{
    return d->customOptions.contains(key);
}

void SearchOptions::enableResultFound(bool enable)
{
    d->enableResultFound = enable;
}

bool SearchOptions::resultFoundEnabled() const
{
    return d->enableResultFound;
}

DFM_SEARCH_END_NS
