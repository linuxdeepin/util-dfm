// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "semanticquerybuilder.h"

#include <dfm-search/filenamesearchapi.h>
#include <dfm-search/contentsearchapi.h>
#include <dfm-search/ocrtextsearchapi.h>
#include <dfm-search/searchfactory.h>
#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

SemanticQueryBuilder::SemanticQueryBuilder() = default;
SemanticQueryBuilder::~SemanticQueryBuilder() = default;

SemanticSearchPlan SemanticQueryBuilder::build(const ParsedIntent &intent)
{
    SemanticSearchPlan plan;

    // Base options shared across all search paths
    SearchOptions baseOpts = buildBaseOptions(intent.timeConstraint);
    baseOpts.setSearchMethod(SearchMethod::Indexed);

    // --- File name search (always enabled) ---
    {
        SearchOptions opts = baseOpts;
        FileNameOptionsAPI fnameApi(opts);
        fnameApi.setPinyinEnabled(true);
        fnameApi.setPinyinAcronymEnabled(true);

        if (!intent.fileExtensions.isEmpty()) {
            fnameApi.setFileExtensions(intent.fileExtensions);
        }

        if (intent.keywords.size() == 1) {
            plan.fileNameQuery = SearchFactory::createQuery(intent.keywords.first());
        } else if (intent.keywords.size() > 1) {
            plan.fileNameQuery = SearchFactory::createQuery(intent.keywords, SearchQuery::Type::Boolean);
        } else {
            // No keywords: search all files (use wildcard to match everything)
            plan.fileNameQuery = SearchFactory::createQuery(QStringLiteral("*"));
        }

        plan.fileNameOptions = opts;
    }

    // --- Content search (when keywords available) ---
    {
        const bool hasKeywords = !intent.keywords.isEmpty();
        bool contentEnabled = hasKeywords;

        // Check if content index is available
        if (contentEnabled && !Global::isContentIndexAvailable()) {
            contentEnabled = false;
        }

        if (contentEnabled) {
            // Check minimum keyword length
            const int minLen = Global::kMinContentSearchKeywordLength;
            bool hasValidKeyword = false;
            for (const QString &kw : intent.keywords) {
                if (kw.length() >= minLen) {
                    hasValidKeyword = true;
                    break;
                }
            }
            if (!hasValidKeyword) {
                contentEnabled = false;
            }
        }

        if (contentEnabled) {
            SearchOptions opts = baseOpts;
            ContentOptionsAPI contentApi(opts);

            // Enable filename-content mixed AND search
            contentApi.setFilenameContentMixedAndSearchEnabled(true);

            if (intent.keywords.size() == 1) {
                plan.contentQuery = SearchFactory::createQuery(intent.keywords.first());
            } else if (intent.keywords.size() > 1) {
                plan.contentQuery = SearchFactory::createQuery(intent.keywords, SearchQuery::Type::Boolean);
            }

            plan.contentOptions = opts;
        }
    }

    // --- OCR search (when keywords available) ---
    {
        const bool hasKeywords = !intent.keywords.isEmpty();
        bool ocrEnabled = hasKeywords;

        if (ocrEnabled && !Global::isOcrTextIndexAvailable()) {
            ocrEnabled = false;
        }

        if (ocrEnabled) {
            SearchOptions opts = baseOpts;
            OcrTextOptionsAPI ocrApi(opts);

            // Enable filename-OCR content mixed AND search
            ocrApi.setFilenameOcrContentMixedAndSearchEnabled(true);

            if (intent.keywords.size() == 1) {
                plan.ocrQuery = SearchFactory::createQuery(intent.keywords.first());
            } else if (intent.keywords.size() > 1) {
                plan.ocrQuery = SearchFactory::createQuery(intent.keywords, SearchQuery::Type::Boolean);
            }

            plan.ocrOptions = opts;
        }
    }

    return plan;
}

TimeRangeFilter SemanticQueryBuilder::buildTimeRangeFilter(const TimeConstraint &tc) const
{
    TimeRangeFilter filter;

    if (!tc.isValid()) {
        return filter;
    }

    switch (tc.kind) {
    case TimeConstraintKind::Preset:
        switch (tc.preset) {
        case TimePreset::Today:
            filter.setToday();
            break;
        case TimePreset::Yesterday:
            filter.setYesterday();
            break;
        case TimePreset::DayBeforeYesterday: {
            const QDate today = QDate::currentDate();
            const QDate dayBefore = today.addDays(-2);
            filter.setRange(QDateTime(dayBefore, QTime(0, 0, 0)),
                            QDateTime(dayBefore, QTime(23, 59, 59)));
            break;
        }
        case TimePreset::ThisWeek:
            filter.setThisWeek();
            break;
        case TimePreset::LastWeek:
            filter.setLastWeek();
            break;
        case TimePreset::ThisMonth:
            filter.setThisMonth();
            break;
        case TimePreset::LastMonth:
            filter.setLastMonth();
            break;
        case TimePreset::ThisYear:
            filter.setThisYear();
            break;
        case TimePreset::LastYear:
            filter.setLastYear();
            break;
        }
        break;
    case TimeConstraintKind::Relative:
        filter.setLast(tc.relativeValue, tc.relativeUnit);
        break;
    case TimeConstraintKind::Custom:
        filter.setRange(tc.customStart, tc.customEnd);
        break;
    case TimeConstraintKind::None:
        break;
    }

    return filter;
}

SearchOptions SemanticQueryBuilder::buildBaseOptions(const TimeConstraint &tc) const
{
    SearchOptions opts;
    const TimeRangeFilter timeFilter = buildTimeRangeFilter(tc);
    if (timeFilter.isValid()) {
        opts.setTimeRangeFilter(timeFilter);
    }
    return opts;
}

DFM_SEARCH_END_NS
