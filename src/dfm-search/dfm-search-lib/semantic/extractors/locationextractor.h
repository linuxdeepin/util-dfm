// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCATIONEXTRACTOR_H
#define LOCATIONEXTRACTOR_H

#include <dfm-search/dimensionextractor.h>

DFM_SEARCH_BEGIN_NS

class SemanticRuleEngine;

class LocationExtractor : public DimensionExtractor
{
public:
    explicit LocationExtractor(SemanticRuleEngine *engine);
    ~LocationExtractor() override;

    void extract(const QString &input, ParsedIntent &intent) override;
    QString name() const override;

    /**
     * @brief Resolve an XDG type string to an absolute filesystem path.
     * @param xdgType One of: "desktop", "download", "documents", "pictures",
     *                "music", "movies", "trash"
     * @return The resolved absolute path, or empty string if unknown
     */
    static QString resolveXdgPath(const QString &xdgType);

private:
    SemanticRuleEngine *m_engine;
};

DFM_SEARCH_END_NS

#endif   // LOCATIONEXTRACTOR_H
