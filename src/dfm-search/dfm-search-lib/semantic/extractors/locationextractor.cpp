// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "locationextractor.h"

#include "semantic/semanticruleengine.h"

#include <QDir>
#include <QFileInfo>
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
        const QString customSubdir = metadata.value("custom_path").toString();
        const bool includeHidden = metadata.value("include_hidden", false).toBool();

        // Always record the consumed span when a rule matches,
        // regardless of whether the resolved directory exists on this machine.
        // This ensures KeywordExtractor correctly excludes trigger words.
        MatchSpan span;
        span.start = m.capturedStart();
        span.end = m.capturedEnd();
        span.ruleId = ruleIds[i];
        intent.consumedSpans.append(span);

        QStringList resolvedPaths;
        if (!customSubdir.isEmpty()) {
            resolvedPaths = expandCustomPath(QDir::homePath(), customSubdir);
        } else {
            const QString path = resolveXdgPath(xdgType);
            if (!path.isEmpty()) {
                resolvedPaths = { path };
            }
        }
        if (resolvedPaths.isEmpty()) {
            continue;
        }

        for (const QString &path : resolvedPaths) {
            if (!QFileInfo::exists(path)) {
                continue;
            }
            if (!intent.searchDirectories.contains(path)) {
                intent.searchDirectories.append(path);
            }
        }

        if (includeHidden) {
            intent.includeHidden = true;
        }
    }
}

QStringList LocationExtractor::expandCustomPath(const QString &basePath, const QString &relPath)
{
    // Reject path traversal segments to prevent escaping basePath
    const QStringList segments = relPath.split(QLatin1String("/"), Qt::SkipEmptyParts);
    if (segments.contains(QLatin1String("..")) || segments.contains(QLatin1String("."))) {
        qWarning() << "Path traversal rejected in custom_path:" << relPath;
        return {};
    }

    const QString normalizedBase = QDir::cleanPath(basePath);
    QStringList candidates = { normalizedBase };

    for (const QString &seg : segments) {
        const bool isGlob = seg.contains(QLatin1Char('*')) || seg.contains(QLatin1Char('?'));
        if (isGlob) {
            QStringList next;
            for (const QString &base : candidates) {
                QDir dir(base);
                const QStringList matches = dir.entryList(
                        QStringList { seg },
                        QDir::Dirs | QDir::NoDotAndDotDot,
                        QDir::Name);
                for (const QString &match : matches) {
                    next.append(base + QLatin1String("/") + match);
                }
            }
            candidates = next;
        } else {
            for (QString &c : candidates) {
                c += QLatin1String("/") + seg;
            }
        }
    }

    // Final guard: verify all results stay within basePath
    const QStringList valid = [&]() {
        QStringList result;
        for (const QString &p : candidates) {
            const QString cleaned = QDir::cleanPath(p);
            if (cleaned.startsWith(normalizedBase + QLatin1String("/"))
                    || cleaned == normalizedBase) {
                result.append(cleaned);
            } else {
                qWarning() << "Path escaped basePath:" << cleaned
                           << "base:" << normalizedBase;
            }
        }
        return result;
    }();

    return valid;
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
