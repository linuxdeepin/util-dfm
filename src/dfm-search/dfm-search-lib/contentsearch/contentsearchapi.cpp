// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/timeresultapi.h>

#include <QDateTime>

DFM_SEARCH_BEGIN_NS

ContentOptionsAPI::ContentOptionsAPI(SearchOptions &options)
    : m_options(options)
{
    // init default
    if (!m_options.hasCustomOption("maxPreviewLength"))
        setMaxPreviewLength(50);
    if (!m_options.hasCustomOption("searchResultHighlight"))
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
    m_options.setCustomOption("searchResultHighlight", enable);
}

bool ContentOptionsAPI::isSearchResultHighlightEnabled() const
{
    return m_options.customOption("searchResultHighlight").toBool();
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

// ==================== Extended Attributes ====================

QString ContentResultAPI::filename() const
{
    return m_result.customAttribute("filename").toString();
}

void ContentResultAPI::setFilename(const QString &name)
{
    m_result.setCustomAttribute("filename", name);
}

bool ContentResultAPI::isHidden() const
{
    return m_result.customAttribute("isHidden").toBool();
}

void ContentResultAPI::setIsHidden(bool hidden)
{
    m_result.setCustomAttribute("isHidden", hidden);
}

// ==================== Modification Time ====================

void ContentResultAPI::setModifyTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("modifyTimestamp", timestamp);
}

qint64 ContentResultAPI::modifyTimestamp() const
{
    return m_result.customAttribute("modifyTimestamp").toLongLong();
}

QString ContentResultAPI::modifyTimeString() const
{
    qint64 ts = modifyTimestamp();
    return ts > 0 ? TimeResultAPI::formatTimestamp(ts) : QString();
}

// ==================== Birth/Creation Time ====================

void ContentResultAPI::setBirthTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("birthTimestamp", timestamp);
}

qint64 ContentResultAPI::birthTimestamp() const
{
    return m_result.customAttribute("birthTimestamp").toLongLong();
}

QString ContentResultAPI::birthTimeString() const
{
    qint64 ts = birthTimestamp();
    return ts > 0 ? TimeResultAPI::formatTimestamp(ts) : QString();
}

DFM_SEARCH_END_NS
