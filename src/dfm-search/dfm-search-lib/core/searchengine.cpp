

// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-search/searchengine.h>
#include <dfm-search/searchfactory.h>

#include "contentsearch/contentsearchengine.h"
#include "filenamesearch/filenamesearchengine.h"

DFM_SEARCH_BEGIN_NS

SearchEngine *SearchEngine::create(SearchType type, QObject *parent)
{
    return SearchFactory::createEngine(type, parent);
}

SearchEngine::SearchEngine(QObject *parent)
    : QObject(parent),
      d_ptr(nullptr)
{
    // 默认创建文件名搜索引擎
    setSearchType(SearchType::FileName);
}

SearchEngine::SearchEngine(SearchType type, QObject *parent)
    : QObject(parent),
      d_ptr(nullptr)
{
    setSearchType(type);
}

SearchEngine::~SearchEngine() = default;

SearchType SearchEngine::searchType() const
{
    if (d_ptr) {
        return d_ptr->searchType();
    }
    return SearchType::FileName;   // 默认
}

void SearchEngine::setSearchType(SearchType type)
{
    // 如果类型相同且引擎已存在，则不重新创建
    if (d_ptr && d_ptr->searchType() == type) {
        return;
    }

    // 根据类型创建相应的引擎
    switch (type) {
    case SearchType::FileName:
        d_ptr = std::make_unique<FileNameSearchEngine>();
        break;
    case SearchType::Content:
        d_ptr = std::make_unique<ContentSearchEngine>();
        break;
    default:
        qWarning("Unsupported search type: %d", static_cast<int>(type));
        return;
    }

    d_ptr->init();

    // 连接信号
    connect(d_ptr.get(), &AbstractSearchEngine::searchStarted,
            this, &SearchEngine::searchStarted);
    connect(d_ptr.get(), &AbstractSearchEngine::resultsFound,
            this, &SearchEngine::resultsFound);
    connect(d_ptr.get(), &AbstractSearchEngine::statusChanged,
            this, &SearchEngine::statusChanged);
    connect(d_ptr.get(), &AbstractSearchEngine::searchFinished,
            this, &SearchEngine::searchFinished);
    connect(d_ptr.get(), &AbstractSearchEngine::searchCancelled,
            this, &SearchEngine::searchCancelled);
    connect(d_ptr.get(), &AbstractSearchEngine::errorOccurred,
            this, &SearchEngine::errorOccurred);
}

SearchOptions SearchEngine::searchOptions() const
{
    if (d_ptr) {
        return d_ptr->searchOptions();
    }
    return SearchOptions();
}

void SearchEngine::setSearchOptions(const SearchOptions &options)
{
    if (d_ptr) {
        d_ptr->setSearchOptions(options);
    }
}

SearchStatus SearchEngine::status() const
{
    if (d_ptr) {
        return d_ptr->status();
    }
    return SearchStatus::Error;
}

void SearchEngine::search(const SearchQuery &query)
{
    if (d_ptr) {
        d_ptr->search(query);
    }
}

void SearchEngine::searchWithCallback(const SearchQuery &query, ResultCallback callback)
{
    if (d_ptr) {
        // 自动开启 resultFoundEnabled 以确保回调能正常工作
        SearchOptions options = d_ptr->searchOptions();
        options.setResultFoundEnabled(true);
        d_ptr->setSearchOptions(options);
        
        d_ptr->searchWithCallback(query, callback);
    }
}

SearchResultExpected SearchEngine::searchSync(const SearchQuery &query)
{
    if (d_ptr) {
        return d_ptr->searchSync(query);
    }
    return QList<SearchResult>();
}

void SearchEngine::cancel()
{
    if (d_ptr) {
        d_ptr->cancel();
    }
}
DFM_SEARCH_END_NS
