// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

#include <fts.h>

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
    bool cancel();

    QList<QSharedPointer<DFileInfo>> fileInfoList();
    bool hasNext();
    QUrl next() const;
    QSharedPointer<DFileInfo> fileInfo() const;
    quint64 fileCount();

    DFMIOError lastError();
    void setErrorFromGError(GError *gerror);

    bool createEnumerator(const QUrl &url, QPointer<DLocalEnumeratorPrivate> me);
    void createEnumneratorAsync(const QUrl &url, QPointer<DLocalEnumeratorPrivate> me, int ioPriority, DEnumerator::InitCallbackFunc func, void *userData);
    void setArguments(const QMap<DEnumerator::ArgumentKey, QVariant> &argus);
    QList<QSharedPointer<DEnumerator::SortFileInfo>> sortFileInfoList();

    static void initAsyncCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData);
    static void freeInitAsyncOp(InitAsyncOp *op);

private:
    bool checkFilter();
    void clean();
    void checkAndResetCancel();
    void insertSortFileInfoList(QList<QSharedPointer<DEnumerator::SortFileInfo>> &fileList,
                                QList<QSharedPointer<DEnumerator::SortFileInfo>> &dirList,
                                FTSENT *ent,
                                FTS *fts, const QSet<QString> &hideList);
    FTS *openDirByfts();

public:
    QList<QSharedPointer<DFileInfo>> list_;
    DLocalEnumerator *q = nullptr;
    GCancellable *cancellable = nullptr;
    QStack<GFileEnumerator *> stackEnumerator;
    QSharedPointer<DFileInfo> dfileInfoNext = nullptr;
    QUrl nextUrl;
    bool enumSubDir = false;
    bool enumLinks = false;
    bool inited = false;
    bool isMixDirAndFile = false;
    std::atomic_bool ftsCanceled = false;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;

    QStringList nameFilters;
    DEnumerator::DirFilters dirFilters = DEnumerator::DirFilter::kNoFilter;
    DEnumerator::IteratorFlags iteratorFlags = DEnumerator::IteratorFlag::kNoIteratorFlags;
    DEnumerator::SortRoleCompareFlag flag = DEnumerator::SortRoleCompareFlag::kSortRoleCompareDefault;

    QMap<QUrl, QSet<QString>> hideListMap;

    QWaitCondition waitCondition;
    QMutex mutex;

    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DLOCALENUMERATOR_P_H
