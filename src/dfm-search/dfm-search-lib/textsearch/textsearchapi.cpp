// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/textsearchapi.h>
#include <dfm-search/timeresultapi.h>

#include <QDateTime>

DFM_SEARCH_BEGIN_NS

// ==================== TextSearchOptionsAPI ====================

TextSearchOptionsAPI::TextSearchOptionsAPI(SearchOptions &options)
    : m_options(options)
{
}

void TextSearchOptionsAPI::setMaxPreviewLength(int length)
{
    m_options.setCustomOption("maxPreviewLength", length);
}

int TextSearchOptionsAPI::maxPreviewLength() const
{
    return m_options.customOption("maxPreviewLength").toInt();
}

void TextSearchOptionsAPI::setSearchResultHighlightEnabled(bool enable)
{
    m_options.setCustomOption("searchResultHighlight", enable);
}

bool TextSearchOptionsAPI::isSearchResultHighlightEnabled() const
{
    return m_options.customOption("searchResultHighlight").toBool();
}

void TextSearchOptionsAPI::setFullTextRetrievalEnabled(bool enable)
{
    m_options.setCustomOption("fullTextRetrieval", enable);
}

bool TextSearchOptionsAPI::isFullTextRetrievalEnabled() const
{
    return m_options.customOption("fullTextRetrieval").toBool();
}

void TextSearchOptionsAPI::setFileExtensions(const QStringList &extensions)
{
    m_options.setCustomOption("fileExtensions", extensions);
}

QStringList TextSearchOptionsAPI::fileExtensions() const
{
    return m_options.customOption("fileExtensions").toStringList();
}

void TextSearchOptionsAPI::setFilenameKeyword(const QString &keyword)
{
    m_options.setCustomOption("filenameKeyword", keyword);
}

QString TextSearchOptionsAPI::filenameKeyword() const
{
    return m_options.customOption("filenameKeyword").toString();
}

// ==================== TextSearchResultAPI ====================

void TextSearchResultAPI::setFileSizeBytes(qint64 bytes)
{
    m_result.setCustomAttribute("fileSizeBytes", bytes);
}

qint64 TextSearchResultAPI::fileSizeBytes() const
{
    return m_result.customAttribute("fileSizeBytes").toLongLong();
}

void TextSearchResultAPI::setCharCount(int count)
{
    m_result.setCustomAttribute("charCount", count);
}

int TextSearchResultAPI::charCount() const
{
    return m_result.customAttribute("charCount").toInt();
}

TextSearchResultAPI::TextSearchResultAPI(SearchResult &result)
    : m_result(result)
{
}

QString TextSearchResultAPI::highlightedContent() const
{
    return m_result.customAttribute("highlightedContent").toString();
}

void TextSearchResultAPI::setHighlightedContent(const QString &content)
{
    m_result.setCustomAttribute("highlightedContent", content);
}

QString TextSearchResultAPI::filename() const
{
    return m_result.customAttribute("filename").toString();
}

void TextSearchResultAPI::setFilename(const QString &name)
{
    m_result.setCustomAttribute("filename", name);
}

bool TextSearchResultAPI::isHidden() const
{
    return m_result.customAttribute("isHidden").toBool();
}

void TextSearchResultAPI::setIsHidden(bool hidden)
{
    m_result.setCustomAttribute("isHidden", hidden);
}

void TextSearchResultAPI::setModifyTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("modifyTimestamp", timestamp);
}

qint64 TextSearchResultAPI::modifyTimestamp() const
{
    return m_result.customAttribute("modifyTimestamp").toLongLong();
}

QString TextSearchResultAPI::modifyTimeString() const
{
    qint64 ts = modifyTimestamp();
    return ts > 0 ? TimeResultAPI::formatTimestamp(ts) : QString();
}

void TextSearchResultAPI::setBirthTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("birthTimestamp", timestamp);
}

qint64 TextSearchResultAPI::birthTimestamp() const
{
    return m_result.customAttribute("birthTimestamp").toLongLong();
}

QString TextSearchResultAPI::birthTimeString() const
{
    qint64 ts = birthTimestamp();
    return ts > 0 ? TimeResultAPI::formatTimestamp(ts) : QString();
}

DFM_SEARCH_END_NS
