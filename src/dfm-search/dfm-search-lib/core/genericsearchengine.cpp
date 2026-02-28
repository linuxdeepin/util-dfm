// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "genericsearchengine.h"

#include <QMutexLocker>
#include <QFileInfo>
#include <QEventLoop>
#include <QTimer>

DFM_SEARCH_BEGIN_NS
DCORE_USE_NAMESPACE

constexpr int kDefaultBatchTime = 1000;   // 1000ms

GenericSearchEngine::GenericSearchEngine(QObject *parent)
    : AbstractSearchEngine(parent),
      m_worker(nullptr)
{
    // 设置初始状态
    m_status.store(SearchStatus::Ready);

    // 设置批处理定时器
    m_batchTimer.setInterval(kDefaultBatchTime);
    connect(&m_batchTimer, &QTimer::timeout, this, [this]() {
        if (!m_batchResults.isEmpty() && m_status.load() == SearchStatus::Searching) {
            emit resultsFound(m_batchResults);
            m_batchResults.clear();
        }
    });
}

GenericSearchEngine::~GenericSearchEngine()
{
    // 停止工作线程
    m_workerThread.quit();
    m_workerThread.wait();

    // 停止批处理定时器
    m_batchTimer.stop();
}

void GenericSearchEngine::init()
{
    // 创建工作对象
    m_worker = new SearchWorker();
    m_worker->moveToThread(&m_workerThread);

    // 连接线程生命周期信号
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // 连接控制信号（主线程 -> 工作线程）
    connect(this, &GenericSearchEngine::requestSearch,
            m_worker, &SearchWorker::doSearch);
    connect(this, &GenericSearchEngine::requestCancel,
            m_worker, &SearchWorker::cancelSearch);

    // 连接结果信号（工作线程 -> 主线程）
    connect(m_worker, &SearchWorker::resultFound,
            this, &GenericSearchEngine::handleSearchResult);
    connect(m_worker, &SearchWorker::searchFinished,
            this, &GenericSearchEngine::handleSearchFinished);
    connect(m_worker, &SearchWorker::errorOccurred,
            this, &GenericSearchEngine::handleErrorOccurred);

    // 设置策略工厂
    setupStrategyFactory();

    // 启动工作线程
    m_workerThread.start();
}

SearchOptions GenericSearchEngine::searchOptions() const
{
    return m_options;
}

void GenericSearchEngine::setSearchOptions(const SearchOptions &options)
{
    m_options = options;
    
    // 更新批处理定时器间隔
    m_batchTimer.setInterval(m_options.batchTime());
}

SearchStatus GenericSearchEngine::status() const
{
    return m_status.load();
}

void GenericSearchEngine::search(const SearchQuery &query)
{
    if (m_status.load() == SearchStatus::Searching)
        return;

    m_cancelled.store(false);
    setStatus(SearchStatus::Searching);
    emit searchStarted();

    // 清空结果列表
    m_results.clear();

    // 清空批处理结果并启动定时器
    m_batchResults.clear();
    m_batchTimer.setInterval(m_options.batchTime());
    m_batchTimer.start();

    // 保存当前查询
    m_currentQuery = query;

    // 验证搜索条件
    auto validationResult = validateSearchConditions();
    if (validationResult.isError()) {
        reportError(validationResult);
        setStatus(SearchStatus::Error);
        return;
    }

    // 发射信号请求工作线程执行搜索
    emit requestSearch(query, m_options, searchType());
}

void GenericSearchEngine::searchWithCallback(const SearchQuery &query,
                                             SearchEngine::ResultCallback callback)
{
    m_callback = callback;
    searchSync(query);
}

SearchResultExpected GenericSearchEngine::searchSync(const SearchQuery &query)
{
    // 保存当前查询
    m_currentQuery = query;

    // 验证搜索条件
    auto validationResult = validateSearchConditions();
    if (validationResult.isError()) {
        return DUnexpected<SearchError>(validationResult);
    }

    // 执行同步搜索
    return doSyncSearch(query);
}

void GenericSearchEngine::cancel()
{
    m_cancelled.store(true);

    // 发射信号请求工作线程取消搜索
    emit requestCancel();

    // 停止批处理定时器
    m_batchTimer.stop();

    if (m_status.load() != SearchStatus::Ready && m_status.load() != SearchStatus::Finished) {
        setStatus(SearchStatus::Cancelled);
        emit searchCancelled();
    }
}

void GenericSearchEngine::handleSearchResult(const DFMSEARCH::SearchResult &result)
{
    // 存储结果到全局结果列表
    m_results.append(result);

    // 如果设置了回调，立即调用回调，如果回调返回true则终止搜索
    if (m_callback) {
        if (m_callback(result)) {
            // 回调返回 true，取消搜索
            cancel();
            return;
        }
    }

    // 将结果添加到批处理队列
    m_batchResults.append(result);
}

void GenericSearchEngine::handleSearchFinished(const DFMSEARCH::SearchResultList &results)
{
    // 停止批处理定时器
    m_batchTimer.stop();

    // 发送剩余的批处理结果
    if (!m_batchResults.isEmpty()) {
        emit resultsFound(m_batchResults);
        m_batchResults.clear();
    }

    // 确保所有结果都已添加到m_results
    if (m_results.size() != results.size()) {
        m_results = results;
    }

    // 设置状态为完成
    setStatus(SearchStatus::Finished);

    // 如果没有通过 handleSearchResult 中断搜索，在这里执行最终检查
    // 注意：通常在 handleSearchResult 已处理大部分情况，此处仅作为备用
    // 发送完成信号
    emit searchFinished(m_results);
}

void GenericSearchEngine::handleErrorOccurred(const DFMSEARCH::SearchError &error)
{
    // 停止批处理定时器
    m_batchTimer.stop();

    // 保存错误
    m_lastError = error;

    // 设置状态
    setStatus(SearchStatus::Error);

    // 转发错误信号
    reportError(error);
}

SearchResultExpected GenericSearchEngine::doSyncSearch(const SearchQuery &query)
{
    // 重置同步搜索状态
    m_results.clear();
    m_lastError = SearchError(SearchErrorCode::Success);

    // 创建事件循环
    QEventLoop eventLoop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(m_options.syncSearchTimeout() * 1000);   // 超时

    // 连接信号到事件循环
    connect(this, &GenericSearchEngine::searchFinished, &eventLoop, &QEventLoop::quit);
    connect(this, &GenericSearchEngine::errorOccurred, &eventLoop, &QEventLoop::quit);
    connect(&timeoutTimer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);

    // 发射信号启动异步搜索
    emit requestSearch(query, m_options, searchType());

    // 启动超时计时器
    timeoutTimer.start();

    // 进入事件循环等待
    eventLoop.exec();

    // 检查是否超时
    if (!timeoutTimer.isActive()) {
        emit requestCancel();
        return DUnexpected<DFMSEARCH::SearchError> { SearchError(SearchErrorCode::SearchTimeout) };
    }

    // 检查是否被取消
    if (m_cancelled.load()) {
        return m_results;
    }

    // 检查是否有错误
    if (m_lastError.isError()) {
        return DUnexpected<DFMSEARCH::SearchError> { m_lastError };
    }

    return m_results;
}

SearchError GenericSearchEngine::validateSearchConditions()
{
    if (m_currentQuery.type() == SearchQuery::Type::Simple || 
        m_currentQuery.type() == SearchQuery::Type::Wildcard) {
        if (m_options.searchPath().isEmpty()) {
            return SearchError(SearchErrorCode::PathIsEmpty);
        }

        // 检查目录是否存在且是否为目录
        QFileInfo pathInfo(m_options.searchPath());
        if (!pathInfo.exists() || !pathInfo.isDir()) {
            return SearchError(SearchErrorCode::PathNotFound);
        }

        // 检查读权限
        if (!pathInfo.isReadable()) {
            return SearchError(SearchErrorCode::PermissionDenied);
        }
    } else if (m_currentQuery.type() == SearchQuery::Type::Boolean) {
        if (m_currentQuery.subQueries().isEmpty())
            return SearchError(SearchErrorCode::InvalidBoolean);
    }

    // 返回空结果列表表示条件有效
    return SearchError(SearchErrorCode::Success);
}

DFM_SEARCH_END_NS
