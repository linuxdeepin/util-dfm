// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ruleconfigloader.h"
#include "semanticruleengine.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSet>
#include <QStandardPaths>

DFM_SEARCH_BEGIN_NS

namespace {
#ifdef CMAKE_INSTALL_PREFIX
constexpr auto kInstallPrefix = CMAKE_INSTALL_PREFIX;
#else
constexpr auto kInstallPrefix = "/usr";
#endif

constexpr auto kLibName = "dfm-search";
}   // namespace

QString RuleConfigLoader::libName()
{
    return QLatin1String(kLibName);
}

QString RuleConfigLoader::systemRulesDir()
{
    return QDir(QDir(QLatin1String(kInstallPrefix))
                        .absoluteFilePath(QLatin1String("share/deepin/")
                                          + libName()
                                          + "/semantic/rules"))
            .absolutePath();
}

QString RuleConfigLoader::userRulesDir()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                + "/deepin/"
                + libName()
                + "/semantic/rules")
            .absolutePath();
}

QString RuleConfigLoader::currentLocaleName()
{
    return QLocale::system().name().simplified();
}

QStringList RuleConfigLoader::ruleFilePaths()
{
    QStringList paths;
    QSet<QString> seen;   // deduplicate by filename

    const QStringList dirs { resolveLocaleDir(userRulesDir()),
                             resolveLocaleDir(systemRulesDir()) };

    for (const QString &dir : dirs) {
        const QStringList files = QDir(dir).entryList(
                QStringList { QStringLiteral("*.json") },
                QDir::Files | QDir::Readable);
        for (const QString &filename : files) {
            const QString absPath = QDir(dir).absoluteFilePath(filename);
            if (!seen.contains(filename) && validateRuleFile(absPath)) {
                paths.append(absPath);
                seen.insert(filename);
            }
        }
    }

    return paths;
}

QString RuleConfigLoader::resolveLocaleDir(const QString &baseDir)
{
    const QString locale = currentLocaleName();

    // Try full locale name (e.g., zh_CN)
    const QString fullLocalePath = QDir(baseDir).absoluteFilePath(locale);
    if (QDir(fullLocalePath).exists()) {
        return fullLocalePath;
    }

    // Try language only (e.g., zh from zh_CN)
    const QString langOnly = locale.split(QLatin1Char('_')).value(0);
    if (!langOnly.isEmpty() && langOnly != locale) {
        const QString langOnlyPath = QDir(baseDir).absoluteFilePath(langOnly);
        if (QDir(langOnlyPath).exists()) {
            return langOnlyPath;
        }
    }

    // Fallback to default locale
    return QDir(baseDir).absoluteFilePath(QLatin1String(kDefaultLocale));
}

QString RuleConfigLoader::resolveRulePath(const QString &filename)
{
    // User-local override with locale
    const QString userLocaleDir = resolveLocaleDir(userRulesDir());
    const QString userPath = QDir(userLocaleDir).absoluteFilePath(filename);
    if (QFile::exists(userPath) && validateRuleFile(userPath)) {
        return userPath;
    }

    // System rules with locale
    const QString sysLocaleDir = resolveLocaleDir(systemRulesDir());
    const QString sysPath = QDir(sysLocaleDir).absoluteFilePath(filename);
    if (QFile::exists(sysPath) && validateRuleFile(sysPath)) {
        return sysPath;
    }

    return {};
}

bool RuleConfigLoader::loadRuleFile(const QString &path, QList<RuleGroup> &groups)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open rule file:" << path;
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in" << path << ":" << parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonArray groupsArray = root.value("groups").toArray();

    for (const QJsonValue &gv : groupsArray) {
        RuleGroup group;
        if (!SemanticRuleEngine::parseRuleGroupStatic(gv.toObject(), group)) {
            qWarning() << "Invalid rule group in" << path;
            continue;
        }
        groups.append(group);
    }

    return !groups.isEmpty();
}

bool RuleConfigLoader::validateRuleFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    const QJsonObject root = doc.object();
    return root.contains("groups") && root.value("groups").isArray();
}

bool RuleConfigLoader::ensureUserRulesDir()
{
    const QString dir = userRulesDir();
    if (!QDir().mkpath(dir)) {
        qWarning() << "Failed to create user rules directory:" << dir;
        return false;
    }
    return true;
}

DFM_SEARCH_END_NS
