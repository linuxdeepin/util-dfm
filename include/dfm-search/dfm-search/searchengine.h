// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QObject>

#include <dfm-search/searchresult.h>
#include <dfm-search/searchoptions.h>
#include <dfm-search/searchquery.h>
#include <dfm-search/searcherror.h>

DFM_SEARCH_BEGIN_NS

class AbstractSearchEngine;

/**
 * @brief 搜索引擎类
 *
 * 提供统一的搜索接口，内部使用具体的搜索引擎实现
 */

class SearchEngine : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 搜索结果回调函数类型
     */
    using ResultCallback = std::function<void(const SearchResult &)>;

    /**
     * @brief 创建特定类型的搜索引擎
     *
     * 使用工厂模式创建引擎，这是唯一创建SearchEngine的方法
     * 返回的对象由Qt父子关系管理生命周期
     */
    static SearchEngine *create(SearchType type, QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SearchEngine() override;

    /**
     * @brief 获取搜索类型
     */
    SearchType searchType() const;

    /**
     * @brief 设置搜索类型
     */
    void setSearchType(SearchType type);

    /**
     * @brief 获取搜索选项
     */
    SearchOptions searchOptions() const;

    /**
     * @brief 设置搜索选项
     */
    void setSearchOptions(const SearchOptions &options);

    /**
     * @brief 获取当前搜索状态
     */
    SearchStatus status() const;

    /**
     * @brief 异步执行搜索
     *
     * @param query 搜索查询
     * @return
     */
    void search(const SearchQuery &query);

    /**
     * @brief 异步执行搜索，并通过回调返回结果
     *
     * @param query 搜索查询
     * @param callback 结果回调函数
     * @return
     */
    void searchWithCallback(const SearchQuery &query, ResultCallback callback);

    /**
     * @brief 同步执行搜索
     *
     * @param query 搜索查询
     * @return 搜索结果列表
     */
    SearchResultExpected searchSync(const SearchQuery &query);

    /**
     * @brief 取消当前搜索
     */
    void cancel();

Q_SIGNALS:
    /**
     * @brief 搜索开始信号
     */
    void searchStarted();

    /**
     * @brief 搜索结果信号
     */
    void resultFound(const DFMSEARCH::SearchResult &result);

    /**
     * @brief 搜索进度信号
     *
     * @param current 当前进度
     * @param total 总进度
     */
    void progressChanged(int current, int total);

    /**
     * @brief 搜索状态改变信号
     */
    void statusChanged(SearchStatus status);

    /**
     * @brief 搜索完成信号
     *
     * @param results 搜索结果列表
     */
    void searchFinished(const DFMSEARCH::SearchResultList &results);

    /**
     * @brief 搜索取消信号
     */
    void searchCancelled();

    // TODO: better error
    /**
     * @brief 搜索错误信号
     *
     * @param message 错误消息
     */
    void errorOccurred(const DFMSEARCH::SearchError &error);

protected:
    explicit SearchEngine(QObject *parent = nullptr);
    SearchEngine(SearchType type, QObject *parent = nullptr);

    friend class SearchFactory;

private:
    std::unique_ptr<AbstractSearchEngine> d_ptr;
};

DFM_SEARCH_END_NS

#endif   // SEARCHENGINE_H
