// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "targetextractor.h"

#include "semantic/semanticruleengine.h"

DFM_SEARCH_BEGIN_NS

TargetExtractor::TargetExtractor(SemanticRuleEngine *engine)
    : m_engine(engine)
{
}

TargetExtractor::~TargetExtractor() = default;

void TargetExtractor::extract(const QString &input, ParsedIntent &intent)
{
    if (!m_engine->hasGroup("target")) {
        return;
    }

    QStringList ruleIds;
    const QList<QRegularExpressionMatch> matches = m_engine->matchAll("target", input, &ruleIds);

    for (int i = 0; i < matches.size(); ++i) {
        MatchSpan span;
        span.setStart(matches[i].capturedStart());
        span.setEnd(matches[i].capturedEnd());
        span.setRuleId(ruleIds[i]);
        intent.consumedSpans().append(span);
    }
}

QString TargetExtractor::name() const
{
    return QStringLiteral("target");
}

DFM_SEARCH_END_NS
