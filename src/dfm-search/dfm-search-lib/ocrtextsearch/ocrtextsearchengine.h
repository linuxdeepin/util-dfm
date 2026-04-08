// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef OCRTEXT_SEARCH_ENGINE_H
#define OCRTEXT_SEARCH_ENGINE_H

#include "core/genericsearchengine.h"

DFM_SEARCH_BEGIN_NS

/**
 * @brief OCR text search engine
 *
 * Implements search functionality for text extracted from images using OCR.
 * This engine uses Lucene indexes similar to content search, but with
 * simplified features (no highlighting support).
 */
class OcrTextSearchEngine : public GenericSearchEngine
{
    Q_OBJECT

public:
    explicit OcrTextSearchEngine(QObject *parent = nullptr);
    ~OcrTextSearchEngine() override;

    // Implement search type
    SearchType searchType() const override { return SearchType::Ocr; }

protected:
    // Setup strategy factory
    void setupStrategyFactory() override;

    // Override validation to add specific checks
    SearchError validateSearchConditions() override;
};

/**
 * @brief OCR text search strategy factory
 */
class OcrTextSearchStrategyFactory : public SearchStrategyFactory
{
public:
    std::unique_ptr<BaseSearchStrategy> createStrategy(
            SearchType searchType, const SearchOptions &options) override;
};

DFM_SEARCH_END_NS

#endif   // OCRTEXT_SEARCH_ENGINE_H
