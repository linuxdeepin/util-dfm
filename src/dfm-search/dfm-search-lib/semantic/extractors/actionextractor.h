// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ACTIONEXTRACTOR_H
#define ACTIONEXTRACTOR_H

#include <dfm-search/dimensionextractor.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;

class ActionExtractor : public DimensionExtractor
{
public:
    explicit ActionExtractor(SemanticRuleEngine *engine);
    ~ActionExtractor() override;

    void extract(const QString &input, ParsedIntent &intent) override;
    QString name() const override;

private:
    SemanticRuleEngine *m_engine;
};

DFM_SEARCH_END_NS

#endif // ACTIONEXTRACTOR_H
