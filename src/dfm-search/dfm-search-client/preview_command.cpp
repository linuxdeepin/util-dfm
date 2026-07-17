// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "preview_command.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

namespace dfmsearch {

namespace {

struct PreviewEntry
{
    QString path;
    DFMSEARCH::PreviewStatus status = DFMSEARCH::PreviewStatus::Success;
    QString content;
    int charCount = 0;
    int keywordOffset = -1;
};

QString formatPreviewNotIndexedError(const QStringList &paths)
{
    QString text;
    QTextStream stream(&text);
    stream << "Error: preview requires indexed content for:\n";
    for (const QString &path : paths) {
        stream << "  " << path << "\n";
    }
    return text;
}

PreviewEntry toPreviewEntry(const QString &path, const DFMSEARCH::PreviewResult &result)
{
    PreviewEntry entry;
    entry.path = path;
    entry.status = result.status();
    entry.content = result.content();
    entry.charCount = result.charCount();
    entry.keywordOffset = result.keywordOffset();
    return entry;
}

QList<PreviewEntry> fetchPreviewEntries(const QStringList &paths,
                                        const SearchCliConfig &config,
                                        DFMSEARCH::ContentRetriever &retriever,
                                        const DFMSEARCH::PreviewOptions &options,
                                        QStringList *notIndexedPaths)
{
    QList<PreviewEntry> entries;
    entries.reserve(paths.size());

    for (const QString &path : paths) {
        DFMSEARCH::PreviewResult result = retriever.fetchPreview(path, config.searchType, options);
        if (result.status() == DFMSEARCH::PreviewStatus::NotIndexed) {
            notIndexedPaths->append(path);
        }
        entries.append(toPreviewEntry(path, result));
    }

    return entries;
}

QString buildPreviewJson(const QList<PreviewEntry> &entries,
                         const SearchCliConfig &config,
                         const DFMSEARCH::PreviewOptions &options)
{
    QJsonObject root;
    root["type"] = "preview";
    root["searchType"] = (config.searchType == SearchType::Content) ? "content"
        : (config.searchType == SearchType::Ocr) ? "ocr" : "semantic";
    root["keyword"] = config.keyword;
    root["offset"] = config.offset;
    root["maxLength"] = options.maxLength();

    QJsonArray results;
    for (const PreviewEntry &entry : entries) {
        QJsonObject item;
        item["path"] = entry.path;
        item["content"] = entry.content;
        item["charCount"] = entry.charCount;
        item["keywordOffset"] = entry.keywordOffset;
        results.append(item);
    }

    root["totalResults"] = results.size();
    root["results"] = results;

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QString buildPreviewText(const QList<PreviewEntry> &entries)
{
    QString text;
    QTextStream out(&text);
    for (const PreviewEntry &entry : entries) {
        out << entry.path << "\n";
        if (!entry.content.isEmpty()) {
            out << "  " << entry.content << "\n";
        } else {
            out << "  (no content)\n";
        }
        out << "  Char count: " << entry.charCount << "\n";
        out << Qt::endl;
    }
    return text;
}

}   // namespace

PreviewCommandResult runPreviewCommand(const SearchCliConfig &config,
                                       DFMSEARCH::ContentRetriever &retriever)
{
    DFMSEARCH::PreviewOptions options;
    options.setOffset(config.offset);
    if (config.keyword.isEmpty() && !config.maxPreviewSet) {
        options.setMaxLength(0);
    } else {
        options.setMaxLength(config.maxPreviewLength);
    }
    options.setKeyword(config.keyword);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    const QStringList paths = config.searchPath.split(',', Qt::SkipEmptyParts);
#else
    const QStringList paths = config.searchPath.split(',', QString::SkipEmptyParts);
#endif

    QStringList notIndexedPaths;
    const QList<PreviewEntry> entries = fetchPreviewEntries(paths, config, retriever, options, &notIndexedPaths);

    PreviewCommandResult commandResult;
    if (!notIndexedPaths.isEmpty()) {
        commandResult.exitCode = 1;
        commandResult.stderrText = formatPreviewNotIndexedError(notIndexedPaths);
        return commandResult;
    }

    if (config.jsonOutput) {
        commandResult.stdoutText = buildPreviewJson(entries, config, options);
    } else {
        commandResult.stdoutText = buildPreviewText(entries);
    }

    return commandResult;
}

}   // namespace dfmsearch
