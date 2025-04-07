#ifndef FILENAME_SEARCH_ENGINE_H
#define FILENAME_SEARCH_ENGINE_H

#include <memory>

#include "core/abstractsearchengine.h"

// 前向声明
// TODO: class LuceneSearchEngine;

DFM_SEARCH_BEGIN_NS

/**
 * @brief 文件名搜索引擎
 *
 * 实现基于文件名的搜索功能
 */
class FileNameSearchEngine : public AbstractSearchEngine
{
    Q_OBJECT

public:
    explicit FileNameSearchEngine(QObject *parent = nullptr);
    ~FileNameSearchEngine() override;

    // 实现AbstractSearchEngine接口
    SearchType searchType() const override { return SearchType::FileName; }

    SearchOptions searchOptions() const override;
    void setSearchOptions(const SearchOptions &options) override;

    SearchStatus status() const override;

    void search(const SearchQuery &query) override;
    void searchWithCallback(const SearchQuery &query,
                            SearchEngine::ResultCallback callback) override;
    SearchResultExpected searchSync(const SearchQuery &query) override;

    void cancel() override;

private:
    // TODO : std::unique_ptr<LuceneSearchEngine> m_engine;
    SearchOptions m_options;
    SearchQuery m_currentQuery;   // 当前查询
};

DFM_SEARCH_END_NS

#endif   // FILENAME_SEARCH_ENGINE_H
