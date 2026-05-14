// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DIMENSIONEXTRACTOR_H
#define DIMENSIONEXTRACTOR_H

#include <dfm-search/semantic_types.h>

DFM_SEARCH_BEGIN_NS

class DimensionExtractor
{
public:
    virtual ~DimensionExtractor() = default;

    /**
     * @brief Extract a dimension from the input text and populate the intent.
     * @param input The raw natural language input
     * @param intent The intent to populate with extracted data
     */
    virtual void extract(const QString &input, ParsedIntent &intent) = 0;

    /**
     * @brief Get the name of this extractor for debugging.
     */
    virtual QString name() const = 0;
};

DFM_SEARCH_END_NS

#endif   // DIMENSIONEXTRACTOR_H
