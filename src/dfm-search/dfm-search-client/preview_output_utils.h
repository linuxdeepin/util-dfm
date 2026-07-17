// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PREVIEW_OUTPUT_UTILS_H
#define PREVIEW_OUTPUT_UTILS_H

#include <dfm-search/contentretriever.h>

#include <QJsonObject>
#include <QTextStream>

namespace dfmsearch {

QJsonObject previewResultToJson(const QString &path, const DFMSEARCH::PreviewResult &result);
void writePreviewResultText(QTextStream &out, const QString &path, const DFMSEARCH::PreviewResult &result);

}   // namespace dfmsearch

#endif   // PREVIEW_OUTPUT_UTILS_H
