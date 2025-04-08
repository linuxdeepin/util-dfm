#include "genericsearchengine.h"

#include <QMutexLocker>
#include <QFileInfo>

DFM_SEARCH_BEGIN_NS
DCORE_USE_NAMESPACE

GenericSearchEngine::GenericSearchEngine(QObject *parent)
    : AbstractSearchEngine(parent),
      m_worker(nullptr),
      m_syncSearchDone(false)
{

    // 设置初始状态
    m_status.store(SearchStatus::Ready);
}

GenericSearchEngine::~GenericSearchEngine()
{
    // 停止工作线程
    m_workerThread.quit();
    m_workerThread.wait();
}

void GenericSearchEngine::init()
{
    // 创建工作对象
    m_worker = new SearchWorker();
    m_worker->moveToThread(&m_workerThread);

    // 连接信号
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // 连接工作对象的信号
    connect(m_worker, &SearchWorker::resultFound,
            this, &GenericSearchEngine::handleSearchResult);
    connect(m_worker, &SearchWorker::searchFinished,
            this, &GenericSearchEngine::handleSearchFinished);
    connect(m_worker, &SearchWorker::progressChanged,
            this, &GenericSearchEngine::handleProgressChanged);
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

    // 保存当前查询
    m_currentQuery = query;

    // 验证搜索条件
    auto validationResult = validateSearchConditions();
    if (validationResult.isError()) {
        reportError(validationResult);
        setStatus(SearchStatus::Error);
        return;
    }

    // 执行异步搜索
    QMetaObject::invokeMethod(m_worker, "doSearch",
                              Q_ARG(DFMSEARCH::SearchQuery, query),
                              Q_ARG(DFMSEARCH::SearchOptions, m_options),
                              Q_ARG(DFMSEARCH::SearchType, searchType()));
}

void GenericSearchEngine::searchWithCallback(const SearchQuery &query,
                                             SearchEngine::ResultCallback callback)
{
    if (m_status.load() == SearchStatus::Searching)
        return;

    m_cancelled.store(false);
    setStatus(SearchStatus::Searching);
    emit searchStarted();

    // 清空结果列表
    m_results.clear();

    // 保存当前查询和回调
    m_currentQuery = query;
    m_callback = callback;

    // 验证搜索条件
    auto validationResult = validateSearchConditions();
    if (validationResult.isError()) {
        reportError(validationResult);
        setStatus(SearchStatus::Error);
        return;
    }

    // 执行异步搜索
    QMetaObject::invokeMethod(m_worker, "doSearch",
                              Q_ARG(DFMSEARCH::SearchQuery, query),
                              Q_ARG(DFMSEARCH::SearchOptions, m_options),
                              Q_ARG(DFMSEARCH::SearchType, searchType()));
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

    // 通知工作线程取消搜索
    QMetaObject::invokeMethod(m_worker, "cancelSearch");

    if (m_status.load() != SearchStatus::Ready && m_status.load() != SearchStatus::Finished) {
        setStatus(SearchStatus::Cancelled);
        emit searchCancelled();
    }
}

void GenericSearchEngine::handleSearchResult(const DFMSEARCH::SearchResult &result)
{
    // 存储结果
    m_results.append(result);

    // 如果设置了回调，调用回调
    if (m_callback) {
        m_callback(result);
    }

    // 发送结果信号
    emit resultFound(result);
}

void GenericSearchEngine::handleSearchFinished(const DFMSEARCH::SearchResultList &results)
{
    QMutexLocker locker(&m_mutex);

    // 更新完成状态
    m_syncSearchDone = true;

    // 确保所有结果都已添加到m_results
    if (m_results.size() != results.size()) {
        m_results = results;
    }

    // 设置状态为完成
    setStatus(SearchStatus::Finished);

    // 发送完成信号
    emit searchFinished(m_results);

    // 唤醒等待的线程（如果有）
    m_waitCond.wakeAll();
}

void GenericSearchEngine::handleProgressChanged(int current, int total)
{
    // 转发进度信号
    reportProgress(current, total);
}

void GenericSearchEngine::handleErrorOccurred(const DFMSEARCH::SearchError &error)
{
    QMutexLocker locker(&m_mutex);

    // 保存错误
    m_lastError = error;

    // 设置状态
    setStatus(SearchStatus::Error);

    // 转发错误信号
    reportError(error);

    // 唤醒等待的线程（如果有）
    m_syncSearchDone = true;
    m_waitCond.wakeAll();
}

SearchResultExpected GenericSearchEngine::doSyncSearch(const SearchQuery &query)
{
    QMutexLocker locker(&m_mutex);

    // 重置同步搜索状态
    m_syncSearchDone = false;
    m_results.clear();

    // 启动异步搜索，通过条件变量等待完成
    QMetaObject::invokeMethod(m_worker, "doSearch",
                              Q_ARG(DFMSEARCH::SearchQuery, query),
                              Q_ARG(DFMSEARCH::SearchOptions, m_options),
                              Q_ARG(DFMSEARCH::SearchType, searchType()));

    // 等待搜索完成或超时
    while (!m_syncSearchDone) {
        // 最多等待30秒
        // TODO (search): config
        if (!m_waitCond.wait(&m_mutex, 30000)) {
            // 超时
            QMetaObject::invokeMethod(m_worker, "cancelSearch");
            return DUnexpected<DFMSEARCH::SearchError> { SearchError(SearchErrorCode::SearchTimeout) };
        }

        // 检查是否被取消
        if (m_cancelled.load()) {
            return m_results;
        }
    }

    // 检查是否有错误
    if (m_lastError.isError()) {
        return DUnexpected<DFMSEARCH::SearchError> { m_lastError };
    }

    return m_results;
}

SearchError GenericSearchEngine::validateSearchConditions()
{
    if (m_currentQuery.type() == SearchQuery::Type::Simple) {
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
