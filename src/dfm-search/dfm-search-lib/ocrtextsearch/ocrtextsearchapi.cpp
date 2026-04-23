// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/ocrtextsearchapi.h>

DFM_SEARCH_BEGIN_NS

OcrTextOptionsAPI::OcrTextOptionsAPI(SearchOptions &options)
    : TextSearchOptionsAPI(options)
{
    // init default
    if (!m_options.hasCustomOption("maxPreviewLength"))
        setMaxPreviewLength(200);
    if (!m_options.hasCustomOption("searchResultHighligh"))
        setSearchResultHighlightEnabled(false);
    if (!m_options.hasCustomOption("fullTextRetrieval"))
        setFullTextRetrievalEnabled(true);
    if (!m_options.hasCustomOption("filenameOcrContentMixedAndSearchEnabled"))
        setFilenameOcrContentMixedAndSearchEnabled(false);
}

void OcrTextOptionsAPI::setFilenameOcrContentMixedAndSearchEnabled(bool enabled)
{
    m_options.setCustomOption("filenameOcrContentMixedAndSearchEnabled", enabled);
}

bool OcrTextOptionsAPI::isFilenameOcrContentMixedAndSearchEnabled() const
{
    return m_options.customOption("filenameOcrContentMixedAndSearchEnabled").toBool();
}

OcrTextResultAPI::OcrTextResultAPI(SearchResult &result)
    : TextSearchResultAPI(result)
{
}

QString OcrTextResultAPI::ocrContent() const
{
    return m_result.customAttribute("ocrContent").toString();
}

void OcrTextResultAPI::setOcrContent(const QString &content)
{
    m_result.setCustomAttribute("ocrContent", content);
}

DFM_SEARCH_END_NS
