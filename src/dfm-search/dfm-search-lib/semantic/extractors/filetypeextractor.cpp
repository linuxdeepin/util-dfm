// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filetypeextractor.h"

#include "semantic/semanticruleengine.h"

DFM_SEARCH_BEGIN_NS

FileTypeExtractor::FileTypeExtractor(SemanticRuleEngine *engine)
    : m_engine(engine)
{
}

FileTypeExtractor::~FileTypeExtractor() = default;

void FileTypeExtractor::extract(const QString &input, ParsedIntent &intent)
{
    if (!m_engine->hasGroup("filetype")) {
        return;
    }

    QStringList ruleIds;
    const QList<QRegularExpressionMatch> matches = m_engine->matchAll("filetype", input, &ruleIds);

    QSet<QString> seenExtensions;

    for (int i = 0; i < matches.size(); ++i) {
        const QRegularExpressionMatch &m = matches[i];
        const QVariantMap metadata = m_engine->ruleMetadata("filetype", ruleIds[i]);

        const QStringList extensions = metadata.value("extensions").toStringList();
        const bool isGeneral = metadata.value("general", false).toBool();

        // If this is a general/fallback type but we already have specific extensions,
        // skip to avoid over-specificity dilution
        if (isGeneral && !seenExtensions.isEmpty()) {
            continue;
        }

        for (const QString &ext : extensions) {
            if (!seenExtensions.contains(ext)) {
                seenExtensions.insert(ext);
            }
        }

        MatchSpan span;
        span.setStart(m.capturedStart());
        span.setEnd(m.capturedEnd());
        span.setRuleId(ruleIds[i]);
        intent.consumedSpans().append(span);
    }

    intent.setFileExtensions(seenExtensions.values());
}

QString FileTypeExtractor::name() const
{
    return QStringLiteral("filetype");
}

DFM_SEARCH_END_NS
