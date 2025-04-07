#ifndef GENERICSEARCHENGINE_H
#define GENERICSEARCHENGINE_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "abstractsearchengine.h"
#include "searchstrategy/searchworker.h"

DFM_SEARCH_BEGIN_NS

/**
 * @brief 通用搜索引擎基类
 *
 * 所有类型搜索引擎的基类实现
 */
class GenericSearchEngine : public AbstractSearchEngine
{
    Q_OBJECT

public:
    explicit GenericSearchEngine(QObject *parent = nullptr);
    virtual ~GenericSearchEngine();
    // 初始化工作线程
    virtual void init() override;

    // 实现AbstractSearchEngine核心接口
    SearchOptions searchOptions() const override;
    void setSearchOptions(const SearchOptions &options) override;
    SearchStatus status() const override;

    void search(const SearchQuery &query) override;
    void searchWithCallback(const SearchQuery &query,
                            SearchEngine::ResultCallback callback) override;
    SearchResultExpected searchSync(const SearchQuery &query) override;

    void cancel() override;

protected:
    // 设置策略工厂
    virtual void setupStrategyFactory() = 0;

    // 执行同步搜索
    SearchResultExpected doSyncSearch(const SearchQuery &query);

    // 检查搜索条件
    virtual SearchResultExpected validateSearchConditions(const SearchQuery &query);

private Q_SLOTS:
    void handleSearchResult(const DFMSEARCH::SearchResult &result);
    void handleSearchFinished(const DFMSEARCH::SearchResultList &results);
    void handleProgressChanged(int current, int total);
    void handleErrorOccurred(const DFMSEARCH::SearchError &error);

protected:
    SearchOptions m_options;
    SearchQuery m_currentQuery;
    SearchEngine::ResultCallback m_callback;
    SearchResultList m_results;

    QThread m_workerThread;
    SearchWorker *m_worker;
    QMutex m_mutex;
    QWaitCondition m_waitCond;
    bool m_syncSearchDone;
    SearchError m_lastError;
};

DFM_SEARCH_END_NS

#endif   // GENERICSEARCHENGINE_H
