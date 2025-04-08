// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef FILENAMESEARCHAPI_H
#define FILENAMESEARCHAPI_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchresult.h>

DFM_SEARCH_BEGIN_NS

/**
 * @brief 文件名搜索API
 *
 * 提供文件名搜索特有的选项设置
 */
class FileNameOptionsAPI
{
public:
    /**
     * @brief 构造函数
     *
     * @param options 要操作的搜索选项对象
     */
    explicit FileNameOptionsAPI(SearchOptions &options);

    /**
     * @brief 设置是否启用拼音搜索
     */
    void setPinyinEnabled(bool enabled);

    /**
     * @brief 获取是否启用拼音搜索
     */
    bool pinyinEnabled() const;

    /**
     * @brief 设置文件类型过滤
     *
     * all types: app, archive, audio, doc, pic, video
     */
    void setFileTypes(const QStringList &types);

    /**
     * @brief 获取文件类型过滤
     */
    QStringList fileTypes() const;

private:
    SearchOptions &m_options;
};

/**
 * @brief 文件名搜索结果API
 *
 */
class FileNameResultAPI
{
public:
    /**
     * @brief 构造函数
     *
     * @param options
     */
    explicit FileNameResultAPI(SearchResult &result);

    QString size() const;
    void setSize(const QString &QString);

    QString modifiedTime() const;
    void setModifiedTime(const QString &time);

    bool isDirectory() const;
    void setIsDirectory(bool isDir);

    QString fileType() const;
    void setFileType(const QString &type) const;

private:
    SearchResult &m_result;
};

DFM_SEARCH_END_NS

#endif   // FILENAMESEARCHAPI_H
