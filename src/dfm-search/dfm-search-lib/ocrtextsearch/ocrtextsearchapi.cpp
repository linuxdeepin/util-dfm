// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/ocrtextsearchapi.h>

DFM_SEARCH_BEGIN_NS

OcrTextOptionsAPI::OcrTextOptionsAPI(SearchOptions &options)
    : m_options(options)
{
    // init default
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
    : m_result(result)
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
