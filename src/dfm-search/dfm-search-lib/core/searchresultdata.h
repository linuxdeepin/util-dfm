#ifndef SEARCH_RESULT_DATA_H
#define SEARCH_RESULT_DATA_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief SearchResult的基本私有实现类
 *
 * 只包含所有搜索结果类型共有的数据
 */
class SearchResultData
{
public:
    SearchResultData();
    SearchResultData(const QString &path);
    SearchResultData(const SearchResultData &other);
    ~SearchResultData() = default;
    SearchResultData(SearchResultData &&other) noexcept;
    SearchResultData &operator=(SearchResultData &&other) noexcept;

    // 公共数据字段
    QString path;
    QVariantMap customAttributes;
};

DFM_SEARCH_END_NS

#endif   // SEARCH_RESULT_DATA_H
