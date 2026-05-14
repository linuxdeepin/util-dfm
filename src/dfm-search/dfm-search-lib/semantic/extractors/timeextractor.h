// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIMEEXTRACTOR_H
#define TIMEEXTRACTOR_H

#include <QMap>

#include <dfm-search/dimensionextractor.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;

class TimeExtractor : public DimensionExtractor
{
public:
    explicit TimeExtractor(SemanticRuleEngine *engine);
    ~TimeExtractor() override;

    void extract(const QString &input, ParsedIntent &intent) override;
    QString name() const override;

private:
    void parseCustomTime(const QRegularExpressionMatch &match, const QVariantMap &metadata, TimeConstraint &tc);
    void parseRelativeTime(const QVariantMap &metadata, TimeConstraint &tc);

    /**
     * @brief Convert a string to int using locale-aware digit mapping.
     *
     * First tries direct integer conversion (Arabic numerals).
     * Falls back to digit_map lookup and positional tens-unit parsing.
     * @param input The string to convert
     * @param digitMap Mapping of locale-specific digit characters to integers (from rule metadata)
     * @param tensUnit The character representing the tens place (from rule metadata, e.g. "十")
     * @return The integer value, or -1 if conversion fails
     */
    static int localeAwareToInt(const QString &input,
                                 const QMap<QString, int> &digitMap,
                                 const QString &tensUnit);

    /**
     * @brief Convert a QVariantMap (from JSON) to a QMap<QString, int>.
     */
    static QMap<QString, int> mapFromVariant(const QVariant &variant);

    SemanticRuleEngine *m_engine;
};

DFM_SEARCH_END_NS

#endif   // TIMEEXTRACTOR_H
