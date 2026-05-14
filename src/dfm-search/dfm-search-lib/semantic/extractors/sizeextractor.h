// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIZEEXTRACTOR_H
#define SIZEEXTRACTOR_H

#include <dfm-search/dimensionextractor.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;

class SizeExtractor : public DimensionExtractor
{
public:
    explicit SizeExtractor(SemanticRuleEngine *engine);
    ~SizeExtractor() override;

    void extract(const QString &input, ParsedIntent &intent) override;
    QString name() const override;

private:
    static qint64 parseSizeToBytes(const QString &value, const QString &unit);
    SemanticRuleEngine *m_engine;
};

DFM_SEARCH_END_NS

#endif // SIZEEXTRACTOR_H
