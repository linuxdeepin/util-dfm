// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RULECONFIGLOADER_H
#define RULECONFIGLOADER_H

#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

struct RuleGroup;

/**
 * @brief Loads semantic rule configuration files from system/user paths.
 *
 * Locale resolution: rules are organized under locale subdirectories
 * (e.g., rules/zh_CN/, rules/en_US/). The loader resolves the locale
 * at runtime using QLocale and falls back through a chain:
 *   zh_CN -> zh -> zh_CN (default)
 *
 * Priority: user-local config > system-installed config.
 * System path: /usr/share/deepin/dfm-search/semantic/rules/
 * User path: ~/.config/deepin/dfm-search/semantic/rules/
 */
class RuleConfigLoader
{
public:
    /**
     * @brief Get the library name based on Qt version.
     */
    static QString libName();

    /**
     * @brief Get the system-installed rules directory.
     */
    static QString systemRulesDir();

    /**
     * @brief Get the user-local rules directory for overrides.
     */
    static QString userRulesDir();

    /**
     * @brief Get the current locale name (e.g., "zh_CN").
     * Uses QLocale::system().name().simplified().
     */
    static QString currentLocaleName();

    /**
     * @brief Scan locale directories and return resolved paths for all rule files.
     * Scans user dir first, then system dir; user files take priority.
     * Falls back through locale chain: zh_CN -> zh -> zh_CN (default).
     * @return Deduplicated list of resolved absolute paths
     */
    static QStringList ruleFilePaths();

    /**
     * @brief Resolve the effective path for a single rule file.
     * Checks user dir first, then system dir, with locale subdirectory lookup.
     * Falls back to zh_CN if the current locale directory is not found.
     * @param filename The rule file name (e.g., "time_rules.json")
     * @return The resolved absolute path, or empty if not found
     */
    static QString resolveRulePath(const QString &filename);

    /**
     * @brief Load and parse a rule file into groups.
     * @param path Absolute path to the JSON file
     * @param groups Output: parsed rule groups
     * @return true if file was loaded and valid
     */
    static bool loadRuleFile(const QString &path, QList<RuleGroup> &groups);

    /**
     * @brief Validate JSON structure of a rule file.
     * @param path Absolute path to the JSON file
     * @return true if valid
     */
    static bool validateRuleFile(const QString &path);

    /**
     * @brief Ensure user rules directory exists.
     * @return true on success
     */
    static bool ensureUserRulesDir();

private:
    /**
     * @brief Get the locale subdirectory name with fallback chain.
     * Tries: full locale (zh_CN) -> language only (zh) -> default (zh_CN)
     * @param baseDir The base rules directory
     * @return The locale subdirectory path that exists, or baseDir/defaultLocale
     */
    static QString resolveLocaleDir(const QString &baseDir);

    static constexpr const char *kDefaultLocale = "zh_CN";
};

DFM_SEARCH_END_NS

#endif   // RULECONFIGLOADER_H
