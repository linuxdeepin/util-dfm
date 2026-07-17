// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_output_utils.h"

namespace dfmsearch {

QJsonObject previewResultToJson(const QString &path, const DFMSEARCH::PreviewResult &result)
{
    QJsonObject item;
    item["path"] = path;
    item["content"] = result.content();
    item["charCount"] = result.charCount();
    item["keywordOffset"] = result.keywordOffset();
    return item;
}

void writePreviewResultText(QTextStream &out, const QString &path, const DFMSEARCH::PreviewResult &result)
{
    out << path << "\n";
    if (!result.content().isEmpty()) {
        out << "  " << result.content() << "\n";
    } else {
        out << "  (no content)\n";
    }
    out << "  Char count: " << result.charCount() << "\n";
    out << Qt::endl;
}

}   // namespace dfmsearch
