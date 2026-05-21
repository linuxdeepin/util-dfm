// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "searchdconfig.h"

#include <DConfig>
#include <QDebug>
#include <QDir>
#include <QObject>
#include <QThread>
#include <QCoreApplication>

DFM_SEARCH_BEGIN_NS

namespace {

std::optional<QStringList> tryLoadStringListFromDConfig(const QString &appId,
                                                         const QString &schemaId,
                                                         const QString &keyName)
{
    QObject dconfigParent;
    auto *dconfigPtr = Dtk::Core::DConfig::create(appId, schemaId, "", &dconfigParent);

    if (!dconfigPtr) {
        qWarning() << "DConfig: Failed to create instance for appId:" << appId << "schemaId:" << schemaId;
        return std::nullopt;
    }

    if (!dconfigPtr->isValid()) {
        qWarning() << "DConfig: Instance is invalid for appId:" << appId << "schemaId:" << schemaId;
        return std::nullopt;
    }

    QVariant value = dconfigPtr->value(keyName);

    if (!value.isValid()) {
        qDebug() << "DConfig: Key '" << keyName << "' not found in appId:" << appId << "schemaId:" << schemaId;
        return std::nullopt;
    }

    if (!value.canConvert<QStringList>()) {
        qWarning() << "DConfig: Value for key '" << keyName << "' in appId:" << appId << "schemaId:" << schemaId
                   << "cannot be converted to QStringList. Actual type:" << value.typeName();
        return std::nullopt;
    }

    return value.toStringList();
}

QStringList resolveIndexedDirectories()
{
    const QString homePath = QDir::homePath();
    const QStringList fallbackResult { homePath };

    auto dconfigNamesOpt = tryLoadStringListFromDConfig("org.deepin.anything", "org.deepin.anything", "indexing_paths");

    if (!dconfigNamesOpt) {
        qDebug() << "Failed to load indexing_paths from DConfig, using fallback.";
        return fallbackResult;
    }

    const QStringList &rawPathsFromDConfig = *dconfigNamesOpt;

    if (rawPathsFromDConfig.isEmpty()) {
        qDebug() << "DConfig provided an empty list for indexing_paths, using fallback.";
        return fallbackResult;
    }

    QStringList processedPaths;
    QSet<QString> uniquePathsChecker;
    bool homePathEncounteredAndAdded = false;

    for (const QString &rawPath : rawPathsFromDConfig) {
        QString currentPath = rawPath.trimmed();

        if (currentPath == QLatin1String("$HOME") || currentPath == QLatin1String("~")) {
            currentPath = homePath;
        } else if (currentPath.startsWith(QLatin1String("$HOME/"))) {
            currentPath = QDir(homePath).filePath(currentPath.mid(6));
        } else if (currentPath.startsWith(QLatin1String("~/"))) {
            currentPath = QDir(homePath).filePath(currentPath.mid(2));
        }

        if (currentPath.isEmpty()) {
            continue;
        }

        currentPath = QDir::cleanPath(currentPath);

        if (currentPath == homePath) {
            if (!homePathEncounteredAndAdded) {
                processedPaths.prepend(currentPath);
                uniquePathsChecker.insert(currentPath);
                homePathEncounteredAndAdded = true;
            }
        } else {
            if (!uniquePathsChecker.contains(currentPath)) {
                processedPaths.append(currentPath);
                uniquePathsChecker.insert(currentPath);
            }
        }
    }

    if (homePathEncounteredAndAdded && !processedPaths.isEmpty() && processedPaths.first() != homePath) {
        processedPaths.removeAll(homePath);
        processedPaths.prepend(homePath);
    }

    if (processedPaths.isEmpty()) {
        return fallbackResult;
    }

    return processedPaths;
}

QStringList deduplicateParentChildPaths(const QStringList &dirs)
{
    if (dirs.isEmpty()) {
        return QStringList();
    }

    QStringList result;
    QStringList normalizedDirs;
    for (const QString &dir : dirs) {
        QString normalizedPath = QDir(dir).absolutePath();
        if (!normalizedPath.endsWith('/')) {
            normalizedPath += '/';
        }
        normalizedDirs.append(normalizedPath);
    }

    std::sort(normalizedDirs.begin(), normalizedDirs.end(),
              [](const QString &a, const QString &b) { return a.length() < b.length(); });

    for (const QString &currentPath : normalizedDirs) {
        bool isSubdirectory = false;
        for (const QString &addedPath : result) {
            if (currentPath.startsWith(addedPath)) {
                isSubdirectory = true;
                break;
            }
        }
        if (!isSubdirectory) {
            QString pathToAdd = currentPath;
            if (pathToAdd.length() > 1 && pathToAdd.endsWith('/')) {
                pathToAdd.chop(1);
            }
            result.append(pathToAdd);
        }
    }

    return result;
}

}   // anonymous namespace

SearchDConfigSnapshot SearchDConfig::loadSnapshot()
{
    // CRITICAL: 此调用必须在主线程中执行（DConfig 通过 DBus 读取，非线程安全）
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    SearchDConfigSnapshot snapshot;

    // 1. 索引目录列表
    snapshot.defaultIndexedDirs = deduplicateParentChildPaths(resolveIndexedDirectories());

    // 2. 黑名单路径
    auto blacklistOpt = tryLoadStringListFromDConfig("org.deepin.anything", "org.deepin.anything", "blacklist_paths");
    if (blacklistOpt) {
        snapshot.defaultBlacklistPaths = *blacklistOpt;
    }

    // 3. 支持的文件扩展名
    auto extensionsOpt = tryLoadStringListFromDConfig("org.deepin.dde.file-manager",
                                                       "org.deepin.dde.file-manager.textindex",
                                                       "supportedFileExtensions");
    if (extensionsOpt) {
        snapshot.supportedFileExtensions = *extensionsOpt;
    }

    return snapshot;
}

QStringList SearchDConfig::loadFileExtensions()
{
    // CRITICAL: 此调用必须在主线程中执行（DConfig 通过 DBus 读取，非线程安全）
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    // 3. 支持的文件扩展名
    auto extensionsOpt = tryLoadStringListFromDConfig("org.deepin.dde.file-manager",
                                                       "org.deepin.dde.file-manager.textindex",
                                                       "supportedFileExtensions");
    if (extensionsOpt.has_value()) {
        return extensionsOpt.value();
    }

    return {};
}

QStringList SearchDConfig::loadBlacklistPaths()
{
    // CRITICAL: 此调用必须在主线程中执行（DConfig 通过 DBus 读取，非线程安全）
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    // 2. 黑名单路径
    auto blacklistOpt = tryLoadStringListFromDConfig("org.deepin.anything", "org.deepin.anything", "blacklist_paths");
    if (blacklistOpt.has_value()) {
        return blacklistOpt.value();
    }

    return {};
}

QStringList SearchDConfig::loadIndexedDirs()
{
    // CRITICAL: 此调用必须在主线程中执行（DConfig 通过 DBus 读取，非线程安全）
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    // 1. 索引目录列表
    return deduplicateParentChildPaths(resolveIndexedDirectories());
}

DFM_SEARCH_END_NS
