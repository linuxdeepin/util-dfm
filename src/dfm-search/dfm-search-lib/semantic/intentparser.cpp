// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "intentparser.h"

#include "semanticruleengine.h"
#include "extractors/filetypeextractor.h"
#include "extractors/keywordextractor.h"
#include "extractors/sizeextractor.h"
#include "extractors/timeextractor.h"

DFM_SEARCH_BEGIN_NS

IntentParser::IntentParser(SemanticRuleEngine *engine)
    : m_engine(engine)
{
    initDefaultExtractors();
}

IntentParser::~IntentParser() = default;

void IntentParser::parse(const QString &input, ParsedIntent &intent)
{
    for (DimensionExtractor *extractor : m_extractors) {
        extractor->extract(input, intent);
    }
}

void IntentParser::addExtractor(std::unique_ptr<DimensionExtractor> extractor)
{
    m_extractors.push_back(extractor.get());
    m_extractorOwners.push_back(std::move(extractor));
}

QStringList IntentParser::extractorNames() const
{
    QStringList names;
    for (DimensionExtractor *e : m_extractors) {
        names.append(e->name());
    }
    return names;
}

void IntentParser::initDefaultExtractors()
{
    // Order matters: keyword MUST be last (depends on consumedSpans)
    addExtractor(std::make_unique<TimeExtractor>(m_engine));
    addExtractor(std::make_unique<FileTypeExtractor>(m_engine));
    addExtractor(std::make_unique<SizeExtractor>(m_engine));
    addExtractor(std::make_unique<KeywordExtractor>(m_engine));
}

DFM_SEARCH_END_NS
