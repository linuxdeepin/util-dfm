// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef FILENAMEBLACKLISTMATCHER_H
#define FILENAMEBLACKLISTMATCHER_H

#include <QString>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS
namespace Global {
namespace BlacklistMatcher {

QString normalizePathForBlacklistMatch(const QString &path);
bool isPathBlacklisted(const QString &inputPath, const QStringList &blacklistEntries);

}   // namespace BlacklistMatcher
}   // namespace Global
DFM_SEARCH_END_NS

#endif   // FILENAMEBLACKLISTMATCHER_H
