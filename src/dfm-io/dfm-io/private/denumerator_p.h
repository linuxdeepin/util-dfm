// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DENUMERATOR_P_H
#define DENUMERATOR_P_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/denumerator.h>

#include <QList>
#include <QMap>
#include <QSet>
#include <QStack>
#include <QSharedPointer>
#include <QMutex>
#include <QWaitCondition>
#include <QPointer>

#include <gio/gio.h>
#include <fts.h>

BEGIN_IO_NAMESPACE

class DEnumerator;
class DFileInfo;

class DEnumeratorPrivate : public QObject, public QEnableSharedFromThis<DEnumeratorPrivate>
{
    Q_OBJECT
public:
    struct EnumUriData
    {
        QSharedPointer<DEnumeratorPrivate> pointer { nullptr };
        GFileEnumerator *enumerator { nullptr };
    };

public:
    explicit DEnumeratorPrivate(DEnumerator *q);
    ~DEnumeratorPrivate();
    bool init(const QUrl &url);
    bool init();
    void clean();
    bool createEnumerator(const QUrl &url, QPointer<DEnumeratorPrivate> me);
    void checkAndResetCancel();
    void setErrorFromGError(GError *gerror);
    bool checkFilter();
    bool openDirByfts();
    void insertSortFileInfoList(QList<QSharedPointer<DEnumerator::SortFileInfo>> &fileList,
                                QList<QSharedPointer<DEnumerator::SortFileInfo>> &dirList,
                                FTSENT *ent,
                                FTS *fts, const QSet<QString> &hideList);
    void enumUriAsyncOvered(GList *files);
    void startAsyncIterator();
    bool hasNext();
    QList<QSharedPointer<DFileInfo>> fileInfoList();
    void setQueryAttributes(const QString &attributes);
    char *filePath(const QUrl &url);
    QUrl buildUrl(const QUrl &url, const char *fileName);

    static void enumUriAsyncCallBack(GObject *sourceObject,
                                     GAsyncResult *res,
                                     gpointer userData);
    static void
    moreFilesCallback(GObject *sourceObject,
                      GAsyncResult *res,
                      gpointer userData);

Q_SIGNALS:
    void asyncIteratorOver();

public:
    DEnumerator *q { nullptr };
    QMutex mutex;
    QWaitCondition waitCondition;
    DFMIOError error;

    GCancellable *cancellable { nullptr };
    QStack<GFileEnumerator *> stackEnumerator;
    QSharedPointer<DFileInfo> dfileInfoNext { nullptr };
    QMap<QUrl, QSet<QString>> hideListMap;
    QList<QSharedPointer<DFileInfo>> infoList;
    QList<GFileInfo *> asyncInfos;
    QString queryAttributes;

    QStringList nameFilters;
    DEnumerator::DirFilters dirFilters { DEnumerator::DirFilter::kNoFilter };
    DEnumerator::IteratorFlags iteratorFlags { DEnumerator::IteratorFlag::kNoIteratorFlags };
    bool isMixDirAndFile { false };
    Qt::SortOrder sortOrder { Qt::AscendingOrder };
    DEnumerator::SortRoleCompareFlag sortRoleFlag { DEnumerator::SortRoleCompareFlag::kSortRoleCompareDefault };

    QUrl uri;
    QUrl nextUrl;
    ulong enumTimeout { 0 };
    bool ftsCanceled { false };
    std::atomic_bool inited { false };
    FTS *fts { nullptr };
    bool enumSubDir { false };
    bool enumLinks { false };
    std::atomic_bool async { false };
    std::atomic_bool asyncStoped { false };
    std::atomic_bool asyncOvered { false };

private:
    bool shouldShowDotAndDotDot(const QString &fileName);
    bool checkEntryTypeFilter();
    bool checkPermissionFilter();
    bool checkSymlinkFilter();
    bool checkHiddenFilter();
    bool checkNameFilter(const QString &fileName);
};

END_IO_NAMESPACE

#endif   // DENUMERATOR_P_H
