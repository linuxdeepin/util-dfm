// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <DExpected>

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searcherror.h>

DFM_SEARCH_BEGIN_NS

class SearchResultData;

class SearchResult
{
public:
    /**
     * @brief 构造函数
     */
    SearchResult();

    /**
     * @brief 使用路径构造
     */
    explicit SearchResult(const QString &path);

    /**
     * @brief 复制构造函数
     */
    SearchResult(const SearchResult &other);

    /**
     * @brief 移动构造函数
     */
    SearchResult(SearchResult &&other) noexcept;

    /**
     * @brief 虚析构函数
     */
    virtual ~SearchResult();

    /**
     * @brief 赋值操作符
     */
    SearchResult &operator=(const SearchResult &other);

    /**
     * @brief 移动赋值操作符
     */
    SearchResult &operator=(SearchResult &&other) noexcept;

    // 基本文件属性
    QString path() const;
    void setPath(const QString &path);

    // 自定义属性
    void setCustomAttribute(const QString &key, const QVariant &value);
    QVariant customAttribute(const QString &key) const;
    bool hasCustomAttribute(const QString &key) const;
    QVariantMap customAttributes() const;

protected:
    std::unique_ptr<SearchResultData> d;
};

using SearchResultList = QList<DFMSEARCH::SearchResult>;
using SearchResultExpected = Dtk::Core::DExpected<QList<SearchResult>, DFMSEARCH::SearchError>;

DFM_SEARCH_END_NS

Q_DECLARE_METATYPE(DFMSEARCH::SearchResult)

#endif   // SEARCHRESULT_H
