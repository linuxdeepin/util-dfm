// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/timeresultapi.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

TimeResultAPI::TimeResultAPI(SearchResult &result)
    : m_result(result)
{
}

void TimeResultAPI::setModifyTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("modifyTimestamp", timestamp);
}

qint64 TimeResultAPI::modifyTimestamp() const
{
    return m_result.customAttribute("modifyTimestamp").toLongLong();
}

QString TimeResultAPI::modifyTimeString() const
{
    qint64 ts = modifyTimestamp();
    return ts > 0 ? formatTimestamp(ts) : QString();
}

void TimeResultAPI::setBirthTimestamp(qint64 timestamp)
{
    m_result.setCustomAttribute("birthTimestamp", timestamp);
}

qint64 TimeResultAPI::birthTimestamp() const
{
    return m_result.customAttribute("birthTimestamp").toLongLong();
}

QString TimeResultAPI::birthTimeString() const
{
    qint64 ts = birthTimestamp();
    return ts > 0 ? formatTimestamp(ts) : QString();
}

QString TimeResultAPI::formatTimestamp(qint64 timestamp)
{
    if (timestamp <= 0) {
        return QString();
    }
    return QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd HH:mm:ss");
}

DFM_SEARCH_END_NS
