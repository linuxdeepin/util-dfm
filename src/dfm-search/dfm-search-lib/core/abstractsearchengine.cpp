// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "abstractsearchengine.h"

DFM_SEARCH_BEGIN_NS

AbstractSearchEngine::AbstractSearchEngine(QObject *parent)
    : QObject(parent),
      m_status(SearchStatus::Ready),
      m_cancelled(false)
{
}

AbstractSearchEngine::~AbstractSearchEngine()
{
}

void AbstractSearchEngine::setStatus(SearchStatus status)
{
    m_status.store(status);
    emit statusChanged(status);
}

void AbstractSearchEngine::reportProgress(int current, int total)
{
    emit progressChanged(current, total);
}

void AbstractSearchEngine::reportError(const QString &message)
{
    emit error(message);
}

DFM_SEARCH_END_NS
