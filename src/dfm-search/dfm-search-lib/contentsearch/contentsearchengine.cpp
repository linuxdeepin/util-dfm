#include "contentsearchengine.h"

DFM_SEARCH_BEGIN_NS

ContentSearchEngine::ContentSearchEngine(QObject *parent)
    : AbstractSearchEngine(parent)
{
    // TODO: 初始化 m_searcher
    // m_searcher = std::make_unique<ContentSearcher>();
}

ContentSearchEngine::~ContentSearchEngine() = default;

SearchOptions ContentSearchEngine::searchOptions() const
{
    return m_options;
}

void ContentSearchEngine::setSearchOptions(const SearchOptions &options)
{
}

SearchStatus ContentSearchEngine::status() const
{
    return m_status.load();
}

void ContentSearchEngine::search(const SearchQuery &query)
{
    if (m_status.load() == SearchStatus::Searching)
        return;

    m_cancelled.store(false);
    setStatus(SearchStatus::Searching);
    emit searchStarted();
}

void ContentSearchEngine::searchWithCallback(const SearchQuery &query, SearchEngine::ResultCallback callback)
{
    if (m_status.load() == SearchStatus::Searching)
        return;

    m_cancelled.store(false);
    setStatus(SearchStatus::Searching);
    emit searchStarted();
}

SearchResultExpected ContentSearchEngine::searchSync(const SearchQuery &query)
{

    // 保存当前查询以便供 convertResults 使用
    m_currentQuery = query;

    // 注：这里只是演示，实际项目中应使用 ContentSearcher 实现
    QList<SearchResult> results;

    if (m_cancelled.load()) {
        return results;
    }

    // TODO: 这里应调用 m_searcher 进行搜索
    // if (m_searcher) {
    //     fakeResults = m_searcher->searchContent(keywords, m_options.searchPath());
    // }

    return results;
}

void ContentSearchEngine::cancel()
{
    m_cancelled.store(true);

    if (m_status.load() != SearchStatus::Ready && m_status.load() != SearchStatus::Finished) {
        setStatus(SearchStatus::Cancelled);
        emit searchCancelled();
    }
}

DFM_SEARCH_END_NS
