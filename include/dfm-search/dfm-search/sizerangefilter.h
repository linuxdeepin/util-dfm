// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SIZERANGEFILTER_H
#define SIZERANGEFILTER_H

#include <cstdint>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class SizeRangeFilterData;

/**
 * @brief The SizeRangeFilter class provides file size range filtering for search operations.
 *
 * This class provides a fluent interface for specifying file size ranges.
 * Size values are in bytes.
 *
 * Example usage:
 * @code
 * // Files between 1KB and 10MB
 * SizeRangeFilter filter;
 * filter.setRange(1024, 10 * 1024 * 1024);
 *
 * // Files larger than 1MB (including 1MB)
 * SizeRangeFilter filter;
 * filter.setMin(1024 * 1024).setIncludeLower(true);
 *
 * // Files smaller than 100KB (excluding 100KB)
 * SizeRangeFilter filter;
 * filter.setMax(100 * 1024).setIncludeUpper(false);
 * @endcode
 */
class SizeRangeFilter
{
public:
    SizeRangeFilter();
    SizeRangeFilter(const SizeRangeFilter &other);
    SizeRangeFilter(SizeRangeFilter &&other) noexcept;
    ~SizeRangeFilter();

    SizeRangeFilter &operator=(const SizeRangeFilter &other);
    SizeRangeFilter &operator=(SizeRangeFilter &&other) noexcept;

    // ---------- Range Setting ----------

    /**
     * @brief Set the minimum file size in bytes
     * @param minSize Minimum file size (0 means no lower bound)
     * @return Reference to this filter for method chaining
     */
    SizeRangeFilter &setMin(qint64 minSize);

    /**
     * @brief Set the maximum file size in bytes
     * @param maxSize Maximum file size (0 means no upper bound)
     * @return Reference to this filter for method chaining
     */
    SizeRangeFilter &setMax(qint64 maxSize);

    /**
     * @brief Set both min and max file size in bytes
     * @param minSize Minimum file size (0 means no lower bound)
     * @param maxSize Maximum file size (0 means no upper bound)
     * @return Reference to this filter for method chaining
     */
    SizeRangeFilter &setRange(qint64 minSize, qint64 maxSize);

    // ---------- Accessors ----------

    /**
     * @brief Get the minimum file size
     * @return Minimum file size in bytes (0 means no lower bound)
     */
    qint64 minSize() const;

    /**
     * @brief Get the maximum file size
     * @return Maximum file size in bytes (0 means no upper bound)
     */
    qint64 maxSize() const;

    // ---------- Boundary Control ----------

    /**
     * @brief Set whether the lower bound is inclusive
     * @param include true to include the lower bound (default: true)
     * @return Reference to this filter for method chaining
     */
    SizeRangeFilter &setIncludeLower(bool include);

    /**
     * @brief Set whether the upper bound is inclusive
     * @param include true to include the upper bound (default: true)
     * @return Reference to this filter for method chaining
     */
    SizeRangeFilter &setIncludeUpper(bool include);

    /**
     * @brief Check if lower bound is inclusive
     * @return true if lower bound is inclusive
     */
    bool includeLower() const;

    /**
     * @brief Check if upper bound is inclusive
     * @return true if upper bound is inclusive
     */
    bool includeUpper() const;

    // ---------- Filter State ----------

    /**
     * @brief Clear the filter (make it invalid)
     * @return Reference to this filter for method chaining
     */
    SizeRangeFilter &clear();

    /**
     * @brief Check if the filter is valid (has at least one bound set)
     * @return true if min or max is set (> 0)
     */
    bool isValid() const;

private:
    std::unique_ptr<SizeRangeFilterData> d;
};

DFM_SEARCH_END_NS

#endif   // SIZERANGEFILTER_H
