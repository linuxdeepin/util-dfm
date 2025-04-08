// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ABSTRACTSEARCHENGINE_H
#define ABSTRACTSEARCHENGINE_H

#include <atomic>

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

#include <dfm-search/searchengine.h>

DFM_SEARCH_BEGIN_NS

class AbstractSearchEngine : public QObject
{
    Q_OBJECT
public:
    explicit AbstractSearchEngine(QObject *parent = nullptr);
    virtual ~AbstractSearchEngine();
    virtual void init();

    // 公共接口方法
    virtual SearchType searchType() const = 0;

    virtual SearchOptions searchOptions() const = 0;
    virtual void setSearchOptions(const SearchOptions &options) = 0;

    virtual SearchStatus status() const = 0;

    virtual void search(const SearchQuery &query) = 0;
    virtual void searchWithCallback(const SearchQuery &query,
                                    SearchEngine::ResultCallback callback) = 0;
    virtual SearchResultExpected searchSync(const SearchQuery &query) = 0;

    virtual void cancel() = 0;

Q_SIGNALS:
    void searchStarted();
    void resultFound(const DFMSEARCH::SearchResult &result);
    void statusChanged(SearchStatus status);
    void searchFinished(const DFMSEARCH::SearchResultList &results);
    void searchCancelled();
    void errorOccurred(const DFMSEARCH::SearchError &error);

protected:
    // 辅助方法
    void setStatus(SearchStatus status);
    void reportError(const SearchError &error);

    std::atomic<SearchStatus> m_status;
    std::atomic<bool> m_cancelled;
};

DFM_SEARCH_END_NS

#endif   // ABSTRACTSEARCHENGINE_H
