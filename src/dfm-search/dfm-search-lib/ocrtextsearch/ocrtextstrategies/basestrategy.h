// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OCRTEXT_BASE_STRATEGY_H
#define OCRTEXT_BASE_STRATEGY_H

#include "core/searchstrategy/basesearchstrategy.h"
#include <dfm-search/ocrtextsearchapi.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief OCR text search strategy base class
 */
class OcrTextBaseStrategy : public BaseSearchStrategy
{
    Q_OBJECT

public:
    explicit OcrTextBaseStrategy(const SearchOptions &options, QObject *parent = nullptr)
        : BaseSearchStrategy(options, parent) { }

    SearchType searchType() const override { return SearchType::Ocr; }
};

DFM_SEARCH_END_NS

#endif   // OCRTEXT_BASE_STRATEGY_H
