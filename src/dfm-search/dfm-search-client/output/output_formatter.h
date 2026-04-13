// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OUTPUT_FORMATTER_H
#define OUTPUT_FORMATTER_H

#include <dfm-search/dsearch_global.h>
#include <dfm-search/searchresult.h>
#include <dfm-search/searchquery.h>

#include <QObject>
#include <QJsonObject>

namespace dfmsearch {

/**
 * @brief 搜索结果输出格式化器基类
 *
 * 定义输出格式化的抽象接口，遵循依赖倒置原则 (DIP)
 */
class OutputFormatter : public QObject
{
    Q_OBJECT

public:
    explicit OutputFormatter(QObject *parent = nullptr)
        : QObject(parent) { }
    virtual ~OutputFormatter() = default;

    /**
     * @brief 设置搜索上下文信息
     */
    virtual void setSearchContext(const QString &keyword, const QString &searchPath,
                                  SearchType searchType, SearchMethod searchMethod) = 0;

    /**
     * @brief 输出搜索开始
     */
    virtual void outputSearchStarted() = 0;

    /**
     * @brief 输出单个搜索结果
     */
    virtual void outputResult(const SearchResult &result) = 0;

    /**
     * @brief 输出搜索结束
     */
    virtual void outputSearchFinished(const QList<SearchResult> &results) = 0;

    /**
     * @brief 输出搜索取消
     */
    virtual void outputSearchCancelled() = 0;

    /**
     * @brief 设置是否启用详细输出模式
     * @param verbose true 启用详细输出， false 只禁用简易输出
     */
    virtual void setVerbose(bool verbose) = 0;

    /**
     * @brief 输出错误
     */
    virtual void outputError(const DFMSEARCH::SearchError &error) = 0;

Q_SIGNALS:
    /**
     * @brief 输出完成信号（用于退出应用）
     */
    void finished();
};

}   // namespace dfmsearch

#endif   // OUTPUT_FORMATTER_H
