// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "actionextractor.h"

#include "../semanticruleengine.h"

#include <QDebug>

DFM_SEARCH_BEGIN_NS

ActionExtractor::ActionExtractor(SemanticRuleEngine *engine)
    : m_engine(engine)
{
}

ActionExtractor::~ActionExtractor() = default;

void ActionExtractor::extract(const QString &input, ParsedIntent &intent)
{
    if (!m_engine->hasGroup("action")) {
        return;
    }

    QString ruleId;
    QRegularExpressionMatch match;
    if (!m_engine->match("action", input, match, &ruleId)) {
        return;
    }

    const QVariantMap metadata = m_engine->ruleMetadata("action", ruleId);
    const QString timeFieldStr = metadata.value("time_field").toString();

    if (timeFieldStr == "birth") {
        intent.timeConstraint.timeField = TimeField::BirthTime;
    } else if (timeFieldStr == "modify") {
        intent.timeConstraint.timeField = TimeField::ModifyTime;
    } else {
        qWarning() << "Unknown time_field in action rule:" << timeFieldStr;
        return;
    }

    MatchSpan span;
    span.start = match.capturedStart();
    span.end = match.capturedEnd();
    span.ruleId = ruleId;
    intent.consumedSpans.append(span);
}

QString ActionExtractor::name() const
{
    return QStringLiteral("action");
}

DFM_SEARCH_END_NS
