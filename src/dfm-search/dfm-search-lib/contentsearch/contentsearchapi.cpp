// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/contentsearchapi.h>

DFM_SEARCH_BEGIN_NS

ContentOptionsAPI::ContentOptionsAPI(SearchOptions &options)
    : m_options(options)
{
    // init default
    if (!m_options.hasCustomOption("maxPreviewLength"))
        setMaxPreviewLength(50);
    if (!m_options.hasCustomOption("searchResultHighligh"))
        setSearchResultHighlightEnabled(false);
    if (!m_options.hasCustomOption("fullTextRetrieval"))
        setFullTextRetrievalEnabled(true);
}

void ContentOptionsAPI::setMaxPreviewLength(int length)
{
    m_options.setCustomOption("maxPreviewLength", length);
}

int ContentOptionsAPI::maxPreviewLength() const
{
    return m_options.customOption("maxPreviewLength").toInt();
}

void ContentOptionsAPI::setSearchResultHighlightEnabled(bool enable)
{
    m_options.setCustomOption("searchResultHighligh", enable);
}

bool ContentOptionsAPI::isSearchResultHighlightEnabled() const
{
    return m_options.customOption("searchResultHighligh").toBool();
}

void ContentOptionsAPI::setFullTextRetrievalEnabled(bool enable)
{
    m_options.setCustomOption("fullTextRetrieval", enable);
}

bool ContentOptionsAPI::isFullTextRetrievalEnabled() const
{
    return m_options.customOption("fullTextRetrieval").toBool();
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
    : m_result(result)
{
}

QString ContentResultAPI::highlightedContent() const
{
    return m_result.customAttribute("highlightedContent").toString();
}

void ContentResultAPI::setHighlightedContent(const QString &content)
{
    m_result.setCustomAttribute("highlightedContent", content);
}

DFM_SEARCH_END_NS
