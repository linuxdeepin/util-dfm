// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "abstractsearchengine.h"

DFM_SEARCH_BEGIN_NS

AbstractSearchEngine::AbstractSearchEngine(QObject *parent)
    : QObject(parent),
      m_status(SearchStatus::Ready),
      m_cancelled(false)
{
    qRegisterMetaType<DFMSEARCH::SearchError>();
    qRegisterMetaType<DFMSEARCH::SearchResult>();
}

AbstractSearchEngine::~AbstractSearchEngine()
{
}

void AbstractSearchEngine::init()
{
}

void AbstractSearchEngine::setStatus(SearchStatus status)
{
    m_status.store(status);
    emit statusChanged(status);
}

void AbstractSearchEngine::reportError(const SearchError &error)
{
    emit errorOccurred(error);
}

DFM_SEARCH_END_NS
