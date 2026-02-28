// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "filenameblacklistmatcher.h"

#include <QDir>

DFM_SEARCH_BEGIN_NS
namespace Global {
namespace BlacklistMatcher {

QString normalizePathForBlacklistMatch(const QString &path)
{
    return QDir::cleanPath(path.trimmed());
}

static bool isAbsolutePathMatch(const QString &normalizedPath, const QString &blacklistAbsolutePath)
{
    const QString normalizedRulePath = normalizePathForBlacklistMatch(blacklistAbsolutePath);
    if (normalizedRulePath.isEmpty()) {
        return false;
    }

    if (normalizedRulePath == "/") {
        return normalizedPath.startsWith('/');
    }

    return normalizedPath == normalizedRulePath || normalizedPath.startsWith(normalizedRulePath + '/');
}

bool isPathBlacklisted(const QString &inputPath, const QStringList &blacklistEntries)
{
    const QString normalizedPath = normalizePathForBlacklistMatch(inputPath);
    const QStringList pathSegments = normalizedPath.split('/', Qt::SkipEmptyParts);

    for (const QString &entry : blacklistEntries) {
        const QString trimmedEntry = entry.trimmed();
        if (trimmedEntry.isEmpty()) {
            continue;
        }

        if (QDir::isAbsolutePath(trimmedEntry)) {
            if (isAbsolutePathMatch(normalizedPath, trimmedEntry)) {
                return true;
            }
            continue;
        }

        if (pathSegments.contains(trimmedEntry)) {
            return true;
        }
    }

    return false;
}

}   // namespace BlacklistMatcher
}   // namespace Global
DFM_SEARCH_END_NS
