#ifndef CONTENT_SEARCH_ENGINE_H
#define CONTENT_SEARCH_ENGINE_H

#include <memory>

#include "core/abstractsearchengine.h"

// 前向声明
class ContentSearcher;

DFM_SEARCH_BEGIN_NS

/**
 * @brief 内容搜索引擎
 *
 * 实现基于文件内容的搜索功能
 */
class ContentSearchEngine : public AbstractSearchEngine
{
    Q_OBJECT

public:
    explicit ContentSearchEngine(QObject *parent = nullptr);
    ~ContentSearchEngine() override;

    // 实现AbstractSearchEngine接口
    SearchType searchType() const override { return SearchType::Content; }
    void setSearchType(SearchType) override { }   // 类型固定为Content

    SearchOptions searchOptions() const override;
    void setSearchOptions(const SearchOptions &options) override;

    SearchStatus status() const override;

    void search(const SearchQuery &query) override;
    void searchWithCallback(const SearchQuery &query,
                            SearchEngine::ResultCallback callback) override;
    QList<SearchResult> searchSync(const SearchQuery &query) override;

    void pause() override;
    void resume() override;
    void cancel() override;

private:
    // TODO:
    // std::unique_ptr<ContentSearcher> m_searcher;
    SearchOptions m_options;
    SearchQuery m_currentQuery;   // 当前查询
};

DFM_SEARCH_END_NS

#endif   // CONTENT_SEARCH_ENGINE_H
