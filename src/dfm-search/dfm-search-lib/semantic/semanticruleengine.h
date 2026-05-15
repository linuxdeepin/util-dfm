// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEMANTICRULEENGINE_H
#define SEMANTICRULEENGINE_H

#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

struct Rule {
    QString id;
    QString pattern;
    QString description;
    bool enabled = true;
    int priority = 0;
    QVariantMap metadata;
    QRegularExpression regex;
};

struct RuleGroup {
    QString name;
    QString version;
    QString locale;
    QList<Rule> rules;
};

/**
 * @brief Rule engine that loads regex rules from JSON config files.
 *
 * Provides match/matchAll operations with priority-based ordering.
 */
class SemanticRuleEngine : public QObject
{
    Q_OBJECT

public:
    explicit SemanticRuleEngine(QObject *parent = nullptr);
    ~SemanticRuleEngine() override;

    /**
     * @brief Load rules from all rule files in the config directory.
     * @return true if at least one valid rule file was loaded.
     */
    bool loadRules();

    /**
     * @brief Load rules from a specific rule file.
     * Useful for testing or loading custom rule files.
     * Merges with any previously loaded rules by group name.
     * @param path Absolute path to a JSON rule file
     * @return true if the file was loaded successfully
     */
    bool loadRuleFile(const QString &path);

    /**
     * @brief Find the highest-priority matching rule in a group.
     * @param group The rule group name
     * @param input The input text to match against
     * @param outMatch Output: the regex match result
     * @param outRuleId Output: the matched rule's ID (optional)
     * @return true if a match was found
     */
    bool match(const QString &group, const QString &input, QRegularExpressionMatch &outMatch,
               QString *outRuleId = nullptr);

    /**
     * @brief Find all matching rules in a group (priority order).
     * @param group The rule group name
     * @param input The input text to match against
     * @param outRuleIds Output: matched rule IDs (optional, parallel to result list)
     * @return List of all matches
     */
    QList<QRegularExpressionMatch> matchAll(const QString &group, const QString &input,
                                              QStringList *outRuleIds = nullptr);

    /**
     * @brief Get a rule's metadata by group and rule ID.
     */
    QVariantMap ruleMetadata(const QString &group, const QString &ruleId) const;

    /**
     * @brief Get all rule IDs in a group.
     */
    QStringList ruleIds(const QString &group) const;

    /**
     * @brief Check if a rule group exists and has enabled rules.
     */
    bool hasGroup(const QString &group) const;

    /**
     * @brief Get the list of loaded rule group names.
     */
    QStringList groupNames() const;

    /**
     * @brief Static helper to parse a rule group from JSON.
     */
    static bool parseRuleGroupStatic(const QJsonObject &groupObj, RuleGroup &outGroup);

private:
    bool parseRuleGroup(const QJsonObject &groupObj, RuleGroup &outGroup);

    QMap<QString, RuleGroup> m_groups;
    QMap<QString, QString> m_ruleFilePaths;    // group name -> resolved file path
};

DFM_SEARCH_END_NS

#endif   // SEMANTICRULEENGINE_H
