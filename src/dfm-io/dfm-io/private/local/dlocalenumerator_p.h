/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lanxuesong<lanxuesong@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DLOCALENUMERATOR_P_H
#define DLOCALENUMERATOR_P_H

#include "dfmio_global.h"
#include "error/error.h"
#include "core/denumerator_p.h"

#include <gio/gio.h>

#include <QList>
#include <QMap>
#include <QSet>
#include <QStack>
#include <QSharedPointer>
#include <QMutex>
#include <QWaitCondition>
#include <QPointer>

BEGIN_IO_NAMESPACE

class DLocalEnumerator;
class DFileInfo;

class DLocalEnumeratorPrivate : public QObject
{
public:
    explicit DLocalEnumeratorPrivate(DLocalEnumerator *q);
    ~DLocalEnumeratorPrivate();

    typedef struct
    {
        QPointer<DLocalEnumeratorPrivate> me;
        DEnumerator::InitCallbackFunc callback;
        gpointer userData;
    } InitAsyncOp;

    bool init(const QUrl &url);
    bool init();
    void initAsync(int ioPriority = 0, DEnumerator::InitCallbackFunc func = nullptr, void *userData = nullptr);

    QList<QSharedPointer<DFileInfo>> fileInfoList();
    bool hasNext();
    QUrl next() const;
    QSharedPointer<DFileInfo> fileInfo() const;
    quint64 fileCount();

    DFMIOError lastError();
    void setErrorFromGError(GError *gerror);

    bool createEnumerator(const QUrl &url, QPointer<DLocalEnumeratorPrivate> me);
    void createEnumneratorAsync(const QUrl &url, QPointer<DLocalEnumeratorPrivate> me, int ioPriority, DEnumerator::InitCallbackFunc func, void *userData);

    static void initAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void freeInitAsyncOp(InitAsyncOp *op);

private:
    bool checkFilter();
    void clean();

public:
    QList<QSharedPointer<DFileInfo>> list_;
    DLocalEnumerator *q = nullptr;
    QStack<GFileEnumerator *> stackEnumerator;
    QSharedPointer<DFileInfo> dfileInfoNext = nullptr;
    QUrl nextUrl;
    bool enumSubDir = false;
    bool enumLinks = false;
    bool inited = false;

    QStringList nameFilters;
    DEnumerator::DirFilters dirFilters = DEnumerator::DirFilter::kNoFilter;
    DEnumerator::IteratorFlags iteratorFlags = DEnumerator::IteratorFlag::kNoIteratorFlags;

    QMap<QUrl, QSet<QString>> hideListMap;

    QWaitCondition waitCondition;
    QMutex mutex;

    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DLOCALENUMERATOR_P_H
