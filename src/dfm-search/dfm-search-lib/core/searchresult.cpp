// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/searchresult.h>

#include "searchresultdata.h"

DFM_SEARCH_BEGIN_NS

/////////
SearchResultData::SearchResultData()
{

}

SearchResultData::SearchResultData(const QString &path)
    : path(path)
{
}

SearchResultData::SearchResultData(const SearchResultData &other)
    : path(other.path),
      customAttributes(other.customAttributes)
{
}

SearchResultData::SearchResultData(SearchResultData &&other) noexcept
    : path(std::move(other.path)),
      customAttributes(std::move(other.customAttributes))
{
}

SearchResultData &SearchResultData::operator=(SearchResultData &&other) noexcept
{
    if (this != &other) {
        path = std::move(other.path);
        customAttributes = std::move(other.customAttributes);
    }
    return *this;
}
/////////

SearchResult::SearchResult()
    : d(std::make_unique<SearchResultData>())
{
}

SearchResult::SearchResult(const QString &path)
    : d(std::make_unique<SearchResultData>(path))
{
}

SearchResult::SearchResult(const SearchResult &other)
    : d(std::make_unique<SearchResultData>(*other.d))
{
}

SearchResult::SearchResult(SearchResult &&other) noexcept
    : d(std::move(other.d))
{
}

SearchResult::~SearchResult() = default;

SearchResult &SearchResult::operator=(const SearchResult &other)
{
    if (this != &other) {
        d = std::make_unique<SearchResultData>(*other.d);
    }
    return *this;
}

SearchResult &SearchResult::operator=(SearchResult &&other) noexcept
{
    if (this != &other) {
        d = std::move(other.d);
    }
    return *this;
}

QString SearchResult::path() const
{
    return d->path;
}

void SearchResult::setPath(const QString &path)
{
    d->path = path;
}

void SearchResult::setCustomAttribute(const QString &key, const QVariant &value)
{
    d->customAttributes[key] = value;
}

QVariant SearchResult::customAttribute(const QString &key) const
{
    return d->customAttributes.value(key);
}

bool SearchResult::hasCustomAttribute(const QString &key) const
{
    return d->customAttributes.contains(key);
}

QVariantMap SearchResult::customAttributes() const
{
    return d->customAttributes;
}

DFM_SEARCH_END_NS
