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

    // Pass location info through to plan (searcher handles per-directory options)
    plan.searchDirectories = intent.searchDirectories;
    plan.includeHidden = intent.includeHidden;

    // Determine time field strategy
    if (intent.timeConstraint.isValid() && intent.timeConstraint.timeField == TimeField::Unspecified) {
        // Time constraint exists but no action specified → search both birth and modify time
        plan.timeField = TimeField::Both;
    } else if (intent.timeConstraint.timeField == TimeField::BirthTime) {
        plan.timeField = TimeField::BirthTime;
    } else if (intent.timeConstraint.timeField == TimeField::ModifyTime) {
        plan.timeField = TimeField::ModifyTime;
    } else {
        plan.timeField = TimeField::ModifyTime;
    }

    // Base options shared across all search paths
    SearchOptions baseOpts = buildBaseOptions(intent.timeConstraint, intent.sizeConstraint);
    baseOpts.setSearchMethod(SearchMethod::Indexed);

    // --- File name search (always enabled) ---
    {
        SearchOptions opts = baseOpts;
        FileNameOptionsAPI fnameApi(opts);

        if (!intent.fileExtensions.isEmpty()) {
            fnameApi.setFileExtensions(intent.fileExtensions);
        }

        if (intent.keywords.size() == 1) {
            plan.fileNameQuery = SearchFactory::createQuery(intent.keywords.first());
        } else if (intent.keywords.size() > 1) {
            plan.fileNameQuery = SearchFactory::createQuery(intent.keywords, SearchQuery::Type::Boolean);
        } else {
            // No keywords: search all files (use wildcard to match everything)
            plan.fileNameQuery = SearchFactory::createQuery("");
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
        filter.setRange(tc.customStart, tc.customEnd);
        break;
    case TimeConstraintKind::Custom:
        filter.setRange(tc.customStart, tc.customEnd);
        break;
    case TimeConstraintKind::None:
        break;
    }

    // Set time field on the filter
    if (tc.timeField == TimeField::BirthTime || tc.timeField == TimeField::ModifyTime) {
        filter.setTimeField(tc.timeField);
    } else if (tc.timeField == TimeField::Unspecified || tc.timeField == TimeField::Both) {
        // No specific time field or both requested → search both birth and modify time
        filter.setTimeField(TimeField::Both);
    }

    return filter;
}

SizeRangeFilter SemanticQueryBuilder::buildSizeRangeFilter(const SizeConstraint &sc) const
{
    SizeRangeFilter filter;
    if (!sc.isValid()) {
        return filter;
    }
    filter.setMin(sc.minSize);
    filter.setMax(sc.maxSize);
    filter.setIncludeLower(sc.includeLower);
    filter.setIncludeUpper(sc.includeUpper);
    return filter;
}

SearchOptions SemanticQueryBuilder::buildBaseOptions(const TimeConstraint &tc, const SizeConstraint &sc) const
{
    SearchOptions opts;
    const TimeRangeFilter timeFilter = buildTimeRangeFilter(tc);
    if (timeFilter.isValid()) {
        opts.setTimeRangeFilter(timeFilter);
    }
    const SizeRangeFilter sizeFilter = buildSizeRangeFilter(sc);
    if (sizeFilter.isValid()) {
        opts.setSizeRangeFilter(sizeFilter);
    }
    return opts;
}

DFM_SEARCH_END_NS
