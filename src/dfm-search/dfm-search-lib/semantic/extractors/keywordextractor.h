// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KEYWORDEXTRACTOR_H
#define KEYWORDEXTRACTOR_H

#include <dfm-search/dimensionextractor.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;

class KeywordExtractor : public DimensionExtractor
{
public:
    explicit KeywordExtractor(SemanticRuleEngine *engine);
    ~KeywordExtractor() override;

    void extract(const QString &input, ParsedIntent &intent) override;
    QString name() const override;

private:
    bool extractStructuredKeywords(const QString &input, ParsedIntent &intent);
    void extractUnconsumedText(const QString &input, ParsedIntent &intent);
    QString extractUnconsumedRegions(const QString &input, const QList<MatchSpan> &allSpans) const;
    static QStringList splitMultiKeywords(const QString &text, const QVariantMap &metadata);

    SemanticRuleEngine *m_engine;
};

DFM_SEARCH_END_NS

#endif   // KEYWORDEXTRACTOR_H
