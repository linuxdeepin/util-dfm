// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/contentsearchapi.h>

DFM_SEARCH_BEGIN_NS

ContentOptionsAPI::ContentOptionsAPI(SearchOptions &options)
    : TextSearchOptionsAPI(options)
{
    // init default
    if (!m_options.hasCustomOption("maxPreviewLength"))
        setMaxPreviewLength(200);
    if (!m_options.hasCustomOption("searchResultHighligh"))
        setSearchResultHighlightEnabled(false);
    if (!m_options.hasCustomOption("fullTextRetrieval"))
        setFullTextRetrievalEnabled(true);
}

void ContentOptionsAPI::setFilenameContentMixedAndSearchEnabled(bool enabled)
{
    m_options.setCustomOption("filenameContentMixedAndSearchEnabled", enabled);
}

bool ContentOptionsAPI::isFilenameContentMixedAndSearchEnabled() const
{
    return m_options.customOption("filenameContentMixedAndSearchEnabled").toBool();
}

ContentResultAPI::ContentResultAPI(SearchResult &result)
    : TextSearchResultAPI(result)
{
}

DFM_SEARCH_END_NS
