#include "filenamesearchengine.h"
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>

DFM_SEARCH_BEGIN_NS

FileNameSearchEngine::FileNameSearchEngine(QObject *parent)
    : AbstractSearchEngine(parent)
{
    // TODO : 实际项目中这里应初始化 m_engine
    // m_engine = std::make_unique<LuceneSearchEngine>();
}

FileNameSearchEngine::~FileNameSearchEngine() = default;

SearchOptions FileNameSearchEngine::searchOptions() const
{
    return m_options;
}

void FileNameSearchEngine::setSearchOptions(const SearchOptions &options)
{
    m_options = options;

    // 配置底层引擎
}

SearchStatus FileNameSearchEngine::status() const
{
    return m_status.load();
}

void FileNameSearchEngine::search(const SearchQuery &query)
{
    if (m_status.load() == SearchStatus::Searching)
        return;

    m_cancelled.store(false);
    setStatus(SearchStatus::Searching);
    emit searchStarted();

    // 保存当前查询以便供 convertResults 使用
    m_currentQuery = query;

    // 实现搜索逻辑...
    // 在实际实现中，可以使用QtConcurrent来异步执行并发出结果信号
}

void FileNameSearchEngine::searchWithCallback(const SearchQuery &query,
                                              SearchEngine::ResultCallback callback)
{
    if (m_status.load() == SearchStatus::Searching)
        return;

    m_cancelled.store(false);
    setStatus(SearchStatus::Searching);
    emit searchStarted();

    // 保存当前查询以便供 convertResults 使用
    m_currentQuery = query;

    // 实现带回调的搜索逻辑...
    // 在实际实现中，可以使用QtConcurrent来异步执行并通过回调返回结果
}

QList<SearchResult> FileNameSearchEngine::searchSync(const SearchQuery &query)
{
    // 保存当前查询以便供 convertResults 使用
    m_currentQuery = query;

    // 注：这里只是演示，实际项目中应使用 LuceneSearchEngine 实现
    QList<SearchResult> results;

    if (m_cancelled.load()) {
        return results;
    }

    return results;
}

void FileNameSearchEngine::cancel()
{
    // TODO: use parent cancel
    m_cancelled.store(true);

    if (m_status.load() != SearchStatus::Ready && m_status.load() != SearchStatus::Finished) {
        setStatus(SearchStatus::Cancelled);
        emit searchCancelled();
    }
}

DFM_SEARCH_END_NS
