// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/ocrtextsearchapi.h>
#include <dfm-search/timeresultapi.h>

#include <QDateTime>

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

// ==================== Extended Attributes ====================

QString OcrTextResultAPI::filename() const
{
    return m_result.customAttribute("filename").toString();
}

void OcrTextResultAPI::setFilename(const QString &name)
{
    m_result.setCustomAttribute("filename", name);
}

bool OcrTextResultAPI::isHidden() const
{
    return m_result.customAttribute("isHidden").toBool();
}

void OcrTextResultAPI::setIsHidden(bool hidden)
{
    m_result.setCustomAttribute("isHidden", hidden);
}

// ==================== Modification Time ====================

void OcrTextResultAPI::setModifyTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("modifyTimestamp", timestamp);
}

qint64 OcrTextResultAPI::modifyTimestamp() const
{
    return m_result.customAttribute("modifyTimestamp").toLongLong();
}

QString OcrTextResultAPI::modifyTimeString() const
{
    qint64 ts = modifyTimestamp();
    return ts > 0 ? TimeResultAPI::formatTimestamp(ts) : QString();
}

// ==================== Birth/Creation Time ====================

void OcrTextResultAPI::setBirthTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("birthTimestamp", timestamp);
}

qint64 OcrTextResultAPI::birthTimestamp() const
{
    return m_result.customAttribute("birthTimestamp").toLongLong();
}

QString OcrTextResultAPI::birthTimeString() const
{
    qint64 ts = birthTimestamp();
    return ts > 0 ? TimeResultAPI::formatTimestamp(ts) : QString();
}

DFM_SEARCH_END_NS
