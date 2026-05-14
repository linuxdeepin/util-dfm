// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "semanticruleengine.h"
#include "ruleconfigloader.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>

DFM_SEARCH_BEGIN_NS

SemanticRuleEngine::SemanticRuleEngine(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_reloadTimer(new QTimer(this))
{
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(100);

    QObject::connect(m_reloadTimer, &QTimer::timeout, this, [this]() {
        loadRules();
        Q_EMIT rulesReloaded();
    });

    QObject::connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, [this](const QString &) {
                m_reloadTimer->start();
            });
}

SemanticRuleEngine::~SemanticRuleEngine() = default;

bool SemanticRuleEngine::loadRules()
{
    QMap<QString, RuleGroup> newGroups;
    QStringList watchedFiles;

    for (const QString &filename : RuleConfigLoader::ruleFileNames()) {
        const QString path = RuleConfigLoader::resolveRulePath(filename);
        if (path.isEmpty()) {
            qWarning() << "Rule file not found:" << filename;
            continue;
        }

        QList<RuleGroup> loaded;
        if (!RuleConfigLoader::loadRuleFile(path, loaded)) {
            qWarning() << "Failed to load rule file:" << path;
            continue;
        }

        for (RuleGroup &group : loaded) {
            if (newGroups.contains(group.name)) {
                // Merge: later rules override by ID
                for (const Rule &rule : group.rules) {
                    auto &existingRules = newGroups[group.name].rules;
                    bool replaced = false;
                    for (int i = 0; i < existingRules.size(); ++i) {
                        if (existingRules[i].id == rule.id) {
                            existingRules[i] = rule;
                            replaced = true;
                            break;
                        }
                    }
                    if (!replaced) {
                        existingRules.append(rule);
                    }
                }
            } else {
                newGroups.insert(group.name, std::move(group));
            }

            m_ruleFilePaths.insert(group.name, path);
            if (!watchedFiles.contains(path)) {
                watchedFiles.append(path);
            }
        }
    }

    if (newGroups.isEmpty()) {
        qWarning() << "No rule files loaded, keeping cached rules";
        return !m_groups.isEmpty();
    }

    // Cache valid rules for rollback
    m_cachedGroups = m_groups.isEmpty() ? newGroups : m_groups;
    m_groups = newGroups;

    // Update file watcher
    if (!m_watcher->files().isEmpty()) {
        m_watcher->removePaths(m_watcher->files());
    }
    for (const QString &f : watchedFiles) {
        m_watcher->addPath(f);
    }

    return true;
}

bool SemanticRuleEngine::loadRuleFile(const QString &path)
{
    QList<RuleGroup> loaded;
    if (!RuleConfigLoader::loadRuleFile(path, loaded)) {
        qWarning() << "Failed to load rule file:" << path;
        return false;
    }

    for (RuleGroup &group : loaded) {
        if (m_groups.contains(group.name)) {
            // Merge: later rules override by ID
            for (const Rule &rule : group.rules) {
                auto &existingRules = m_groups[group.name].rules;
                bool replaced = false;
                for (int i = 0; i < existingRules.size(); ++i) {
                    if (existingRules[i].id == rule.id) {
                        existingRules[i] = rule;
                        replaced = true;
                        break;
                    }
                }
                if (!replaced) {
                    existingRules.append(rule);
                }
            }
        } else {
            m_groups.insert(group.name, std::move(group));
        }

        m_ruleFilePaths.insert(group.name, path);
    }

    return true;
}

bool SemanticRuleEngine::match(const QString &group, const QString &input, QRegularExpressionMatch &outMatch,
                                QString *outRuleId)
{
    if (!m_groups.contains(group)) {
        return false;
    }

    const QList<Rule> &rules = m_groups.value(group).rules;
    QList<Rule> sorted = rules;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Rule &a, const Rule &b) { return a.priority > b.priority; });

    for (const Rule &rule : sorted) {
        if (!rule.enabled || !rule.regex.isValid()) {
            continue;
        }
        QRegularExpressionMatch m = rule.regex.match(input);
        if (m.hasMatch()) {
            outMatch = m;
            if (outRuleId) {
                *outRuleId = rule.id;
            }
            return true;
        }
    }

    return false;
}

QList<QRegularExpressionMatch> SemanticRuleEngine::matchAll(const QString &group, const QString &input,
                                                              QStringList *outRuleIds)
{
    QList<QRegularExpressionMatch> results;

    if (!m_groups.contains(group)) {
        return results;
    }

    const QList<Rule> &rules = m_groups.value(group).rules;
    QList<Rule> sorted = rules;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Rule &a, const Rule &b) { return a.priority > b.priority; });

    for (const Rule &rule : sorted) {
        if (!rule.enabled || !rule.regex.isValid()) {
            continue;
        }

        // Use globalMatch to find ALL occurrences of this rule's pattern.
        // This is important for noise rules (e.g., "和" appearing multiple times).
        auto it = rule.regex.globalMatch(input);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            if (m.hasMatch()) {
                results.append(m);
                if (outRuleIds) {
                    outRuleIds->append(rule.id);
                }
            }
        }
    }

    return results;
}

QVariantMap SemanticRuleEngine::ruleMetadata(const QString &group, const QString &ruleId) const
{
    if (!m_groups.contains(group)) {
        return {};
    }

    for (const Rule &rule : m_groups.value(group).rules) {
        if (rule.id == ruleId) {
            return rule.metadata;
        }
    }
    return {};
}

bool SemanticRuleEngine::hasGroup(const QString &group) const
{
    return m_groups.contains(group);
}

QStringList SemanticRuleEngine::ruleIds(const QString &group) const
{
    const auto it = m_groups.constFind(group);
    if (it == m_groups.constEnd()) {
        return {};
    }

    QStringList ids;
    for (const Rule &rule : it->rules) {
        ids.append(rule.id);
    }
    return ids;
}

QStringList SemanticRuleEngine::groupNames() const
{
    return m_groups.keys();
}

bool SemanticRuleEngine::parseRuleGroupStatic(const QJsonObject &groupObj, RuleGroup &outGroup)
{
    if (!groupObj.contains("name") || !groupObj.contains("rules")) {
        return false;
    }

    outGroup.name = groupObj.value("name").toString();
    outGroup.version = groupObj.value("version").toString("1.0.0");
    outGroup.locale = groupObj.value("locale").toString();

    const QJsonArray rulesArray = groupObj.value("rules").toArray();
    for (const QJsonValue &rv : rulesArray) {
        const QJsonObject ruleObj = rv.toObject();

        Rule rule;
        rule.id = ruleObj.value("id").toString();
        rule.pattern = ruleObj.value("pattern").toString();
        rule.description = ruleObj.value("description").toString();
        rule.enabled = ruleObj.value("enabled").toBool(true);
        rule.priority = ruleObj.value("priority").toInt(0);
        rule.metadata = ruleObj.value("metadata").toVariant().toMap();

        if (rule.pattern.isEmpty() || rule.id.isEmpty()) {
            continue;
        }

        rule.regex.setPattern(rule.pattern);
        rule.regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        if (!rule.regex.isValid()) {
            qWarning() << "Invalid regex for rule" << rule.id << ":" << rule.regex.errorString();
            continue;
        }

        outGroup.rules.append(rule);
    }

    return !outGroup.rules.isEmpty();
}

void SemanticRuleEngine::onRuleFilesChanged(const QStringList &files)
{
    Q_UNUSED(files);
    m_reloadTimer->start();
}

DFM_SEARCH_END_NS
