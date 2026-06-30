// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TARGETEXTRACTOR_H
#define TARGETEXTRACTOR_H

#include <dfm-search/dimensionextractor.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;

class TargetExtractor : public DimensionExtractor
{
public:
    explicit TargetExtractor(SemanticRuleEngine *engine);
    ~TargetExtractor() override;

    void extract(const QString &input, ParsedIntent &intent) override;
    QString name() const override;

private:
    SemanticRuleEngine *m_engine;
};

DFM_SEARCH_END_NS

#endif   // TARGETEXTRACTOR_H
