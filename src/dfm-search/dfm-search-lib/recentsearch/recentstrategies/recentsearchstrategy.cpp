// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "recentsearchstrategy.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

DFM_SEARCH_BEGIN_NS

namespace {
constexpr auto kDBusService = "org.deepin.Filemanager.Daemon.RecentManager";
constexpr auto kDBusPath = "/org/deepin/Filemanager/Daemon/RecentManager";
constexpr auto kDBusInterface = "org.deepin.Filemanager.Daemon.RecentManager";
constexpr auto kDBusMethod = "GetItemsInfo";
}   // namespace

RecentSearchStrategy::RecentSearchStrategy(const SearchOptions &options, QObject *parent)
    : BaseSearchStrategy(options, parent)
{
}

// ── DBus 获取 + JSON 解析 ──────────────────────────────────────────

QList<RecentItem> RecentSearchStrategy::fetchRecentItems()
{
    QList<RecentItem> items;

    QDBusInterface iface(QString::fromLatin1(kDBusService), QString::fromLatin1(kDBusPath),
                         QString::fromLatin1(kDBusInterface),
                         QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qWarning() << "RecentManager DBus interface is invalid:"
                   << iface.lastError().message();
        return items;
    }

    QDBusReply<QString> reply = iface.call(QString::fromLatin1(kDBusMethod));
    if (!reply.isValid()) {
        qWarning() << "RecentManager GetItemsInfo failed:" << reply.error().message();
        return items;
    }

    const QByteArray raw = reply.value().toUtf8();
    if (raw.isEmpty()) {
        return items;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isArray()) {
        qWarning() << "RecentManager: invalid JSON payload:" << parseErr.errorString();
        return items;
    }

    const QJsonArray arr = doc.array();
    items.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        const QJsonObject obj = v.toObject();
        RecentItem item;
        item.href = obj.value("Href").toString();
        item.path = obj.value("Path").toString();
        item.modified = static_cast<qint64>(obj.value("modified").toVariant().toLongLong());
        if (!item.path.isEmpty() && item.modified > 0) {
            items.append(std::move(item));
        }
    }

    return items;
}

// ── 过滤管道 ───────────────────────────────────────────────────────
//
// 三个 filterBy* 纯函数：输入 items + 约束 → 输出子集。
// 行为与 FileNameRealTimeStrategy 中的对应过滤逻辑保持一致，
// 确保语义搜索在不同数据源下产生可预期的匹配结果。

QList<RecentItem> RecentSearchStrategy::filterByKeyword(const QList<RecentItem> &items,
                                                         const QString &keyword) const
{
    if (keyword.isEmpty()) {
        return items;
    }

    const bool caseSensitive = m_options.caseSensitive();
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QList<RecentItem> out;
    for (const RecentItem &item : items) {
        const QString fileName = QFileInfo(item.path).fileName();
        if (fileName.contains(keyword, cs)) {
            out.append(item);
        }
    }
    return out;
}

QList<RecentItem> RecentSearchStrategy::filterByExtensions(const QList<RecentItem> &items,
                                                           const QStringList &extensions) const
{
    if (extensions.isEmpty()) {
        return items;
    }

    QList<RecentItem> out;
    for (const RecentItem &item : items) {
        const QString suffix = QFileInfo(item.path).suffix().toLower();
        if (extensions.contains(suffix)) {
            out.append(item);
        }
    }
    return out;
}

QList<RecentItem> RecentSearchStrategy::filterByTimeRange(const QList<RecentItem> &items) const
{
    const TimeRangeFilter filter = m_options.timeRangeFilter();
    auto [start, end] = filter.resolveTimeRange();

    QList<RecentItem> out;
    for (const RecentItem &item : items) {
        const QDateTime fileTime = QDateTime::fromSecsSinceEpoch(item.modified);

        bool match = true;
        if (start.isValid()) {
            match = filter.includeLower() ? (fileTime >= start) : (fileTime > start);
        }
        if (match && end.isValid()) {
            match = filter.includeUpper() ? (fileTime <= end) : (fileTime < end);
        }
        if (match) {
            out.append(item);
        }
    }
    return out;
}

// ── 结果构建 ───────────────────────────────────────────────────────

SearchResult RecentSearchStrategy::toSearchResult(const RecentItem &item) const
{
    SearchResult result(item.path);
    QFileInfo fi(item.path);

    if (m_options.detailedResultsEnabled()) {
        FileNameResultAPI api(result);
        api.setIsDirectory(fi.isDir());
        if (!fi.isDir()) {
            api.setFileType(fi.suffix().isEmpty() ? QStringLiteral("unknown") : fi.suffix().toLower());
            api.setFileExtension(fi.suffix().toLower());
            api.setSize(QString::number(fi.size()));
            api.setFileSizeBytes(fi.size());
        } else {
            api.setFileType(QStringLiteral("dir"));
        }
        api.setFilename(fi.fileName());
        api.setIsHidden(fi.isHidden());
        api.setModifyTimestamp(item.modified);   // 最近使用时间，非文件系统 mtime
    }

    return result;
}

// ── 主搜索流程 ─────────────────────────────────────────────────────

void RecentSearchStrategy::search(const SearchQuery &query)
{
    m_cancelled.store(false);
    m_results.clear();

    QElapsedTimer timer;
    timer.start();

    // Step 1: 从 DBus 拉取全部最近使用记录
    QList<RecentItem> items = fetchRecentItems();

    if (m_cancelled.load()) {
        emit searchFinished(m_results);
        return;
    }

    // Step 2: 关键词过滤（仅 Simple 类型有 keyword；Boolean/Wildcard 暂不支持）
    QString keyword;
    if (query.type() == SearchQuery::Type::Simple) {
        keyword = query.keyword();
    } else if (query.type() == SearchQuery::Type::Boolean && !query.subQueries().isEmpty()) {
        // 布尔查询取第一个子查询作为关键词（最近记录量小，不做全文布尔）
        keyword = query.subQueries().first().keyword();
    }
    items = filterByKeyword(items, keyword);

    // Step 3: 扩展名过滤
    FileNameOptionsAPI optApi(const_cast<SearchOptions &>(m_options));
    const QStringList exts = optApi.fileExtensions();
    items = filterByExtensions(items, exts);

    // Step 4: 时间范围过滤（基于 modified 时间戳）
    if (m_options.hasTimeRangeFilter()) {
        items = filterByTimeRange(items);
    }

    // Step 5: 构建 SearchResult 并发射
    const bool resultFoundEnabled = m_options.resultFoundEnabled();
    const int maxResults = m_options.maxResults() > 0 ? m_options.maxResults() : INT_MAX;

    int count = 0;
    for (const RecentItem &item : std::as_const(items)) {
        if (m_cancelled.load() || count >= maxResults) {
            break;
        }

        SearchResult result = toSearchResult(item);
        m_results.append(result);
        if (resultFoundEnabled) {
            emit resultFound(result);
        }
        ++count;
    }

    qInfo() << "Recent search completed in" << timer.elapsed() << "ms with" << count << "results";
    emit searchFinished(m_results);
}

void RecentSearchStrategy::cancel()
{
    m_cancelled.store(true);
}

DFM_SEARCH_END_NS
