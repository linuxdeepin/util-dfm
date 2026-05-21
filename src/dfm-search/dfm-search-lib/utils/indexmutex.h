// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef INDEX_MUTEX_H
#define INDEX_MUTEX_H

#include <QMutex>
#include <QMap>

DFM_SEARCH_BEGIN_NS

class IndexMutexGuard
{
public:
    static QMutex &mutexForPath(const QString &indexDir)
    {
        static QMutex s_mapMutex;
        static QMap<QString, QMutex *> s_mutexMap;

        QMutexLocker mapLock(&s_mapMutex);
        auto it = s_mutexMap.find(indexDir);
        if (it == s_mutexMap.end()) {
            it = s_mutexMap.insert(indexDir, new QMutex());
        }
        return *it.value();
    }
};

DFM_SEARCH_END_NS

#endif   // INDEX_MUTEX_H
