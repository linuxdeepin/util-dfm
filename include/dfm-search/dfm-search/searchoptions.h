// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHOPTIONS_H
#define SEARCHOPTIONS_H

#include <QString>
#include <QStringList>
#include <QVariant>

#include <dfm-search/dsearch_global.h>

DFM_SEARCH_BEGIN_NS

class SearchOptionsData;

class SearchOptions
{
public:
    /**
     * @brief 构造函数
     */
    SearchOptions();

    /**
     * @brief 拷贝构造函数
     */
    SearchOptions(const SearchOptions &other);

    /**
     * @brief 移动构造函数
     */
    SearchOptions(SearchOptions &&other) noexcept;

    /**
     * @brief 析构函数
     */
    virtual ~SearchOptions();

    /**
     * @brief 赋值操作符
     */
    SearchOptions &operator=(const SearchOptions &other);

    /**
     * @brief 移动赋值操作符
     */
    SearchOptions &operator=(SearchOptions &&other) noexcept;

    /**
     * @brief 获取搜索方法
     */
    SearchMethod method() const;

    /**
     * @brief 设置搜索方法
     * 索引搜索适合已建立索引的目录，速度快但可能不是最新结果
     * 实时搜索直接扫描文件系统，结果是最新的但速度可能较慢
     */
    void setSearchMethod(SearchMethod method);

    /**
     * @brief 判断是否区分大小写
     */
    bool caseSensitive() const;

    /**
     * @brief 设置是否区分大小写
     */
    void setCaseSensitive(bool sensitive);

    /**
     * @brief 获取起始搜索路径
     */
    QString searchPath() const;

    /**
     * @brief 设置起始搜索路径
     */
    void setSearchPath(const QString &path);

    /**
     * @brief 获取排除路径列表
     */
    QStringList excludePaths() const;

    /**
     * @brief 设置排除路径列表
     */
    void setExcludePaths(const QStringList &paths);

    /**
     * @brief 添加排除路径
     */
    void addExcludePath(const QString &path);

    /**
     * @brief 设置是否包含隐藏文件
     */
    void setIncludeHidden(bool include);

    /**
     * @brief 是否包含隐藏文件
     */
    bool includeHidden() const;

    /**
     * @brief 获取最大结果数量限制
     */
    int maxResults() const;

    /**
     * @brief 设置最大结果数量限制
     */
    void setMaxResults(int count);

    /**
     * @brief 设置自定义选项
     */
    void setCustomOption(const QString &key, const QVariant &value);

    /**
     * @brief 获取自定义选项
     */
    QVariant customOption(const QString &key) const;

    /**
     * @brief 判断是否设置了指定的自定义选项
     */
    bool hasCustomOption(const QString &key) const;

private:
    std::unique_ptr<SearchOptionsData> d;   // PIMPL
};

DFM_SEARCH_END_NS

#endif   // SEARCHOPTIONS_H
