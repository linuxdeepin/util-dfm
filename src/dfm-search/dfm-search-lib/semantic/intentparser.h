// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INTENTPARSER_H
#define INTENTPARSER_H

#include <dfm-search/dimensionextractor.h>
#include <dfm-search/dsearch_global.h>
#include <dfm-search/semantic_types.h>

#include <memory>
#include <vector>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;

/**
 * @brief Orchestrates dimension extractors to parse natural language into intent.
 *
 * Extractors run in order. KeywordExtractor MUST be last
 * because it relies on consumedSpans from earlier extractors.
 */
class IntentParser
{
public:
    explicit IntentParser(SemanticRuleEngine *engine);
    ~IntentParser();

    /**
     * @brief Parse natural language input into a structured intent.
     * @param input The raw natural language string
     * @param intent Output: parsed intent
     */
    void parse(const QString &input, ParsedIntent &intent);

    /**
     * @brief Add a custom dimension extractor.
     * Extractors are called in the order they are added.
     */
    void addExtractor(std::unique_ptr<DimensionExtractor> extractor);

    /**
     * @brief Get the list of extractor names.
     */
    QStringList extractorNames() const;

private:
    void initDefaultExtractors();

    SemanticRuleEngine *m_engine;
    std::vector<DimensionExtractor *> m_extractors;
    std::vector<std::unique_ptr<DimensionExtractor>> m_extractorOwners;
};

DFM_SEARCH_END_NS

#endif   // INTENTPARSER_H
