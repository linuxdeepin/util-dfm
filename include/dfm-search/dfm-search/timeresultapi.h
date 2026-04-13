// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TIMERESULTAPI_H
#define TIMERESULTAPI_H

#include <QString>
#include <QDateTime>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class SearchResult;

/**
 * @brief The TimeResultAPI class provides shared time attribute access for search results
 *
 * This class provides a unified interface for accessing time-related attributes
 * (modification time and birth/creation time) in SearchResult objects. It follows
 * the DRY principle by centralizing time attribute handling across different
 * search types (filename, content, OCR).
 *
 * Time values are stored as Unix timestamps (seconds since epoch) internally,
 * and can be retrieved either as raw timestamps or as formatted time strings.
 */
class TimeResultAPI
{
public:
    /**
     * @brief Constructor
     * @param result The SearchResult object to operate on
     */
    explicit TimeResultAPI(SearchResult &result);

    // ==================== Modification Time ====================

    /**
     * @brief Set the modification time timestamp
     * @param timestamp Unix timestamp in seconds
     */
    void setModifyTimestamp(qint64 timestamp);

    /**
     * @brief Get the modification time timestamp
     * @return Unix timestamp in seconds, 0 if not set
     */
    qint64 modifyTimestamp() const;

    /**
     * @brief Get the modification time as a formatted string
     * @return Formatted time string (yyyy-MM-dd HH:mm:ss), empty if not set
     */
    QString modifyTimeString() const;

    // ==================== Birth/Creation Time ====================

    /**
     * @brief Set the birth/creation time timestamp
     * @param timestamp Unix timestamp in seconds
     */
    void setBirthTimestamp(qint64 timestamp);

    /**
     * @brief Get the birth/creation time timestamp
     * @return Unix timestamp in seconds, 0 if not set
     */
    qint64 birthTimestamp() const;

    /**
     * @brief Get the birth/creation time as a formatted string
     * @return Formatted time string (yyyy-MM-dd HH:mm:ss), empty if not set
     */
    QString birthTimeString() const;

    // ==================== Utility Functions ====================

    /**
     * @brief Format a Unix timestamp to a human-readable string
     * @param timestamp Unix timestamp in seconds
     * @return Formatted time string (yyyy-MM-dd HH:mm:ss)
     */
    static QString formatTimestamp(qint64 timestamp);

private:
    SearchResult &m_result;
};

DFM_SEARCH_END_NS

#endif   // TIMERESULTAPI_H
