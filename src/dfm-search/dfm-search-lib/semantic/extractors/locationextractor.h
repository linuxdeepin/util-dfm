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
     *
     * @note For non-XDG paths (e.g., WeChat), use metadata "custom_path"
     *       instead, which resolves to ~/<custom_path>.
     */
    static QString resolveXdgPath(const QString &xdgType);

    /**
     * @brief Expand a custom_path (may contain glob wildcards) to concrete absolute paths.
     * @param basePath The base directory (e.g., home path)
     * @param relPath  The relative path, may contain * or ? wildcards
     * @return List of absolute paths. Empty if glob matches nothing.
     *
     * Walk relPath segment-by-segment: non-glob segments are appended directly,
     * glob segments are expanded by enumerating the parent directory.
     */
    static QStringList expandCustomPath(const QString &basePath, const QString &relPath);

private:
    SemanticRuleEngine *m_engine;
};

DFM_SEARCH_END_NS

#endif   // LOCATIONEXTRACTOR_H
