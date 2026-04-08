// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ocrtextsearchengine.h"

#include "ocrtextstrategies/indexedstrategy.h"

DFM_SEARCH_BEGIN_NS
DCORE_USE_NAMESPACE

OcrTextSearchEngine::OcrTextSearchEngine(QObject *parent)
    : GenericSearchEngine(parent)
{
}

OcrTextSearchEngine::~OcrTextSearchEngine() = default;

void OcrTextSearchEngine::setupStrategyFactory()
{
    // Set up OCR text search strategy factory
    auto factory = std::make_unique<OcrTextSearchStrategyFactory>();
    m_worker->setStrategyFactory(std::move(factory));
}

SearchError OcrTextSearchEngine::validateSearchConditions()
{
    // First execute base class validation
    auto result = GenericSearchEngine::validateSearchConditions();
    if (result.isError()) {
        return result;
    }

    // OCR text search specific validation
    if (m_options.method() != SearchMethod::Indexed) {
        return SearchError(SearchErrorCode::InvalidSerchMethod);
    }

    // Check for unsupported Wildcard query type
    if (m_currentQuery.type() == SearchQuery::Type::Wildcard) {
        return SearchError(OcrTextSearchErrorCode::WildcardNotSupported);
    }

    if (m_currentQuery.type() == SearchQuery::Type::Simple
        && m_currentQuery.keyword().toUtf8().size() < Global::kMinContentSearchKeywordLength) {
        return SearchError(OcrTextSearchErrorCode::KeywordTooShort);
    }

    return result;
}

std::unique_ptr<BaseSearchStrategy> OcrTextSearchStrategyFactory::createStrategy(
        SearchType searchType, const SearchOptions &options)
{
    // Ensure correct search type
    if (searchType != SearchType::Ocr) {
        return nullptr;
    }

    // Create corresponding strategy based on search method
    if (options.method() == SearchMethod::Indexed) {
        return std::make_unique<OcrTextIndexedStrategy>(options);
    }

    return nullptr;
}

DFM_SEARCH_END_NS
