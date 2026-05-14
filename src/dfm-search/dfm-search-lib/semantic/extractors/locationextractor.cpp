// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "locationextractor.h"

#include "semantic/semanticruleengine.h"

#include <QDir>
#include <QStandardPaths>

DFM_SEARCH_BEGIN_NS

LocationExtractor::LocationExtractor(SemanticRuleEngine *engine)
    : m_engine(engine)
{
}

LocationExtractor::~LocationExtractor() = default;

void LocationExtractor::extract(const QString &input, ParsedIntent &intent)
{
    if (!m_engine->hasGroup("location")) {
        return;
    }

    // Use matchAll to support multiple directory mentions (e.g., "桌面和下载的图片")
    QStringList ruleIds;
    const QList<QRegularExpressionMatch> matches = m_engine->matchAll("location", input, &ruleIds);

    for (int i = 0; i < matches.size(); ++i) {
        const QRegularExpressionMatch &m = matches[i];
        const QVariantMap metadata = m_engine->ruleMetadata("location", ruleIds[i]);

        const QString xdgType = metadata.value("xdg_type").toString();
        const bool includeHidden = metadata.value("include_hidden", false).toBool();

        const QString path = resolveXdgPath(xdgType);
        if (path.isEmpty()) {
            continue;
        }

        if (!intent.searchDirectories.contains(path)) {
            intent.searchDirectories.append(path);
        }

        if (includeHidden) {
            intent.includeHidden = true;
        }

        MatchSpan span;
        span.start = m.capturedStart();
        span.end = m.capturedEnd();
        span.ruleId = ruleIds[i];
        intent.consumedSpans.append(span);
    }
}

QString LocationExtractor::resolveXdgPath(const QString &xdgType)
{
    if (xdgType == QLatin1String("desktop")) {
        return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    if (xdgType == QLatin1String("download")) {
        return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
    if (xdgType == QLatin1String("documents")) {
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    if (xdgType == QLatin1String("pictures")) {
        return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
    if (xdgType == QLatin1String("music")) {
        return QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    }
    if (xdgType == QLatin1String("movies")) {
        return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    }
    if (xdgType == QLatin1String("trash")) {
        return QDir::homePath() + QLatin1String("/.local/share/Trash/files");
    }

    return {};
}

QString LocationExtractor::name() const
{
    return QStringLiteral("location");
}

DFM_SEARCH_END_NS
