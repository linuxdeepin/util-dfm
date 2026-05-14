// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <dfm-search/sizerangefilter.h>

DFM_SEARCH_BEGIN_NS

class SizeRangeFilterData
{
public:
    SizeRangeFilterData()
        : minSize(0), maxSize(0), includeLower(true), includeUpper(true)
    {
    }

    SizeRangeFilterData(const SizeRangeFilterData &other)
        : minSize(other.minSize), maxSize(other.maxSize),
          includeLower(other.includeLower), includeUpper(other.includeUpper)
    {
    }

    qint64 minSize;
    qint64 maxSize;
    bool includeLower;
    bool includeUpper;
};

SizeRangeFilter::SizeRangeFilter()
    : d(std::make_unique<SizeRangeFilterData>())
{
}

SizeRangeFilter::SizeRangeFilter(const SizeRangeFilter &other)
    : d(std::make_unique<SizeRangeFilterData>(*other.d))
{
}

SizeRangeFilter::SizeRangeFilter(SizeRangeFilter &&other) noexcept
    : d(std::move(other.d))
{
}

SizeRangeFilter::~SizeRangeFilter() = default;

SizeRangeFilter &SizeRangeFilter::operator=(const SizeRangeFilter &other)
{
    if (this != &other) {
        d = std::make_unique<SizeRangeFilterData>(*other.d);
    }
    return *this;
}

SizeRangeFilter &SizeRangeFilter::operator=(SizeRangeFilter &&other) noexcept
{
    if (this != &other) {
        d = std::move(other.d);
    }
    return *this;
}

SizeRangeFilter &SizeRangeFilter::setMin(qint64 minSize)
{
    d->minSize = minSize;
    return *this;
}

SizeRangeFilter &SizeRangeFilter::setMax(qint64 maxSize)
{
    d->maxSize = maxSize;
    return *this;
}

SizeRangeFilter &SizeRangeFilter::setRange(qint64 minSize, qint64 maxSize)
{
    d->minSize = minSize;
    d->maxSize = maxSize;
    return *this;
}

qint64 SizeRangeFilter::minSize() const
{
    return d->minSize;
}

qint64 SizeRangeFilter::maxSize() const
{
    return d->maxSize;
}

SizeRangeFilter &SizeRangeFilter::setIncludeLower(bool include)
{
    d->includeLower = include;
    return *this;
}

SizeRangeFilter &SizeRangeFilter::setIncludeUpper(bool include)
{
    d->includeUpper = include;
    return *this;
}

bool SizeRangeFilter::includeLower() const
{
    return d->includeLower;
}

bool SizeRangeFilter::includeUpper() const
{
    return d->includeUpper;
}

SizeRangeFilter &SizeRangeFilter::clear()
{
    d->minSize = 0;
    d->maxSize = 0;
    d->includeLower = true;
    d->includeUpper = true;
    return *this;
}

bool SizeRangeFilter::isValid() const
{
    return d->minSize > 0 || d->maxSize > 0;
}

DFM_SEARCH_END_NS
