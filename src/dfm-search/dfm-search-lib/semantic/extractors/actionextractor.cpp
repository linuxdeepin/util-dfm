// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "actionextractor.h"

#include "semantic/semanticruleengine.h"
#include "locationextractor.h"

#include <QDir>
#include <QFileInfo>
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

    // Use matchAll to support co-existing action dimensions
    // (e.g., "别人传的修改过的文档" = IM received + modify time).
    QStringList ruleIds;
    const QList<QRegularExpressionMatch> matches =
            m_engine->matchAll("action", input, &ruleIds);

    for (int i = 0; i < matches.size(); ++i) {
        const QRegularExpressionMatch &m = matches[i];
        const QVariantMap metadata = m_engine->ruleMetadata("action", ruleIds[i]);
        const QString timeFieldStr = metadata.value("time_field").toString();
        const QString imCategory = metadata.value("im_category").toString();
        const bool includeHidden = metadata.value("include_hidden", false).toBool();
        const bool hiddenOnly = metadata.value("hidden_only", false).toBool();
        const bool recent = metadata.value("recent", false).toBool();

        // ── Recently-used files (DBus RecentManager data source) ──
        if (recent) {
            intent.setRecentOnly(true);
            // Force ModifyTime — recent records only carry a "modified" timestamp
            intent.timeConstraint().setTimeField(TimeField::ModifyTime);

            MatchSpan span;
            span.setStart(m.capturedStart());
            span.setEnd(m.capturedEnd());
            span.setRuleId(ruleIds[i]);
            intent.consumedSpans().append(span);
            continue;
        }

        // ── Hidden-only files (filename search only, include hidden directories) ──
        if (hiddenOnly) {
            intent.setHiddenOnly(true);
            intent.setIncludeHidden(true);
            intent.setSearchTarget(SearchTarget::FileNameOnly);

            MatchSpan span;
            span.setStart(m.capturedStart());
            span.setEnd(m.capturedEnd());
            span.setRuleId(ruleIds[i]);
            intent.consumedSpans().append(span);
            continue;
        }

        // ── Include hidden files ──
        if (includeHidden) {
            intent.setIncludeHidden(true);

            MatchSpan span;
            span.setStart(m.capturedStart());
            span.setEnd(m.capturedEnd());
            span.setRuleId(ruleIds[i]);
            intent.consumedSpans().append(span);
            continue;
        }

        // ── IM category actions (e.g., received → IM directories) ──
        if (imCategory == QLatin1String("received")) {
            const QStringList resolvedPaths = resolveImReceivedPaths();
            if (!resolvedPaths.isEmpty()) {
                for (const QString &path : resolvedPaths) {
                    if (!intent.searchDirectories().contains(path)) {
                        intent.searchDirectories().append(path);
                    }
                }

                MatchSpan span;
                span.setStart(m.capturedStart());
                span.setEnd(m.capturedEnd());
                span.setRuleId(ruleIds[i]);
                intent.consumedSpans().append(span);
            }
            // else: no IM directories resolved → do NOT consume span
            // (trigger words demote to keyword fallback)
            continue;
        }

        // ── Existing time_field actions ──
        if (timeFieldStr == QLatin1String("birth")) {
            intent.timeConstraint().setTimeField(TimeField::BirthTime);
        } else if (timeFieldStr == QLatin1String("modify")) {
            intent.timeConstraint().setTimeField(TimeField::ModifyTime);
        } else {
            qWarning() << "Unknown action metadata in rule" << ruleIds[i]
                       << ":" << metadata;
            continue;
        }

        MatchSpan span;
        span.setStart(m.capturedStart());
        span.setEnd(m.capturedEnd());
        span.setRuleId(ruleIds[i]);
        intent.consumedSpans().append(span);
    }
}

QStringList ActionExtractor::resolveImReceivedPaths() const
{
    QStringList paths;

    if (!m_engine->hasGroup("location")) {
        return paths;
    }

    const QStringList locRuleIds = m_engine->ruleIds("location");
    for (const QString &rid : locRuleIds) {
        const QVariantMap meta = m_engine->ruleMetadata("location", rid);
        if (!meta.value("im_received").toBool()) {
            continue;
        }

        const QString xdgType = meta.value("xdg_type").toString();
        const QString customSubdir = meta.value("custom_path").toString();

        QStringList resolved;
        if (!customSubdir.isEmpty()) {
            resolved = LocationExtractor::expandCustomPath(
                    QDir::homePath(), customSubdir);
        } else if (!xdgType.isEmpty()) {
            const QString path = LocationExtractor::resolveXdgPath(xdgType);
            if (!path.isEmpty()) {
                resolved = { path };
            }
        }

        for (const QString &p : resolved) {
            if (QFileInfo::exists(p) && !paths.contains(p)) {
                paths.append(p);
            }
        }
    }

    return paths;
}

QString ActionExtractor::name() const
{
    return QStringLiteral("action");
}

DFM_SEARCH_END_NS
