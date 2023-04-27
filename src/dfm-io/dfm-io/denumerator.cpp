// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/denumerator_p.h"

#include "utils/dlocalhelper.h"

#include <dfm-io/denumerator.h>
#include <dfm-io/dfileinfo.h>
#include <dfm-io/denumeratorfuture.h>

#include <QVariant>
#include <QPointer>
#include <QtConcurrent>
#include <QDebug>
#include <qobjectdefs.h>

#include <sys/stat.h>

#define FILE_DEFAULT_ATTRIBUTES "standard::*,etag::*,id::*,access::*,mountable::*,time::*,unix::*,dos::*,\
owner::*,thumbnail::*,preview::*,filesystem::*,gvfs::*,selinux::*,trash::*,recent::*,metadata::*"

USING_IO_NAMESPACE

/************************************************
 * DEnumeratorPrivate
 ***********************************************/

DEnumeratorPrivate::DEnumeratorPrivate(DEnumerator *q)
    : q(q)
{
}

DEnumeratorPrivate::~DEnumeratorPrivate()
{
    clean();
    if (cancellable) {
        g_object_unref(cancellable);
        cancellable = nullptr;
    }
}

bool DEnumeratorPrivate::init(const QUrl &url)
{
    QPointer<DEnumeratorPrivate> me = this;
    const bool needTimeOut = q->timeout() != 0;
    if (!needTimeOut) {
        return createEnumerator(url, me);
    } else {
        mutex.lock();
        bool succ = false;
        QtConcurrent::run([this, me, url, &succ]() {
            succ = createEnumerator(url, me);
        });
        bool wait = waitCondition.wait(&mutex, q->timeout());
        mutex.unlock();
        if (!wait)
            qWarning() << "createEnumeratorInThread failed, url: " << url << " error: " << error.errorMsg();
        return succ && wait;
    }
}

bool DEnumeratorPrivate::init()
{
    const QUrl &uri = q->uri();
    bool ret = init(uri);
    inited = true;
    return ret;
}

void DEnumeratorPrivate::clean()
{
    if (!stackEnumerator.isEmpty()) {
        while (true) {
            GFileEnumerator *enumerator = stackEnumerator.pop();
            g_object_unref(enumerator);
            if (stackEnumerator.isEmpty())
                break;
        }
    }
}

bool DEnumeratorPrivate::createEnumerator(const QUrl &url, QPointer<DEnumeratorPrivate> me)
{
    const QString &uriPath = url.toString();
    g_autoptr(GFile) gfile = g_file_new_for_uri(uriPath.toLocal8Bit().data());

    g_autoptr(GError) gerror = nullptr;
    checkAndResetCancel();
    GFileEnumerator *genumerator = g_file_enumerate_children(gfile,
                                                             FILE_DEFAULT_ATTRIBUTES,
                                                             enumLinks ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                             cancellable,
                                                             &gerror);
    if (!me) {
        error.setCode(DFMIOErrorCode(DFM_IO_ERROR_NOT_FOUND));
        return false;
    }
    bool ret = true;
    if (!genumerator || gerror) {
        if (gerror)
            setErrorFromGError(gerror);
        ret = false;
        qWarning() << "create enumerator failed, url: " << uriPath << " error: " << error.errorMsg() << gerror->message;
    } else {
        stackEnumerator.push_back(genumerator);
    }
    waitCondition.wakeAll();
    return ret;
}

void DEnumeratorPrivate::checkAndResetCancel()
{
    if (cancellable) {
        g_object_unref(cancellable);
        cancellable = nullptr;
    }
    cancellable = g_cancellable_new();
}

void DEnumeratorPrivate::setErrorFromGError(GError *gerror)
{
    if (!gerror)
        return;

    error.setCode(DFMIOErrorCode(gerror->code));
    if (error.code() == DFMIOErrorCode::DFM_IO_ERROR_FAILED)
        error.setMessage(gerror->message);
}

bool DEnumeratorPrivate::checkFilter()
{
    if (dirFilters.testFlag(DEnumerator::DirFilter::kNoFilter))
        return true;

    if (!dfileInfoNext)
        return false;

    const bool isDir = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool();
    if ((dirFilters & DEnumerator::DirFilter::kAllDirs).testFlag(DEnumerator::DirFilter::kAllDirs)) {   // all dir, no apply filters rules
        if (isDir)
            return true;
    }

    // dir filter
    bool ret = true;

    const bool readable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanRead).toBool();
    const bool writable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanWrite).toBool();
    const bool executable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanExecute).toBool();

    auto checkRWE = [&]() -> bool {
        if ((dirFilters & DEnumerator::DirFilter::kReadable).testFlag(DEnumerator::DirFilter::kReadable)) {
            if (!readable)
                return false;
        }
        if ((dirFilters & DEnumerator::DirFilter::kWritable).testFlag(DEnumerator::DirFilter::kWritable)) {
            if (!writable)
                return false;
        }
        if ((dirFilters & DEnumerator::DirFilter::kExecutable).testFlag(DEnumerator::DirFilter::kExecutable)) {
            if (!executable)
                return false;
        }
        return true;
    };

    if ((dirFilters & DEnumerator::DirFilter::kAllEntries).testFlag(DEnumerator::DirFilter::kAllEntries)
        || ((dirFilters & DEnumerator::DirFilter::kDirs) && (dirFilters & DEnumerator::DirFilter::kFiles))) {
        // 判断读写执行
        if (!checkRWE())
            ret = false;
    } else if ((dirFilters & DEnumerator::DirFilter::kDirs).testFlag(DEnumerator::DirFilter::kDirs)) {
        if (!isDir) {
            ret = false;
        } else {
            // 判断读写执行
            if (!checkRWE())
                ret = false;
        }
    } else if ((dirFilters & DEnumerator::DirFilter::kFiles).testFlag(DEnumerator::DirFilter::kFiles)) {
        const bool isFile = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsFile).toBool();
        if (!isFile) {
            ret = false;
        } else {
            // 判断读写执行
            if (!checkRWE())
                ret = false;
        }
    }

    if ((dirFilters & DEnumerator::DirFilter::kNoSymLinks).testFlag(DEnumerator::DirFilter::kNoSymLinks)) {
        const bool isSymlinks = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool();
        if (isSymlinks)
            ret = false;
    }

    const QString &fileInfoName = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardName).toString();
    const bool showHidden = (dirFilters & DEnumerator::DirFilter::kHidden).testFlag(DEnumerator::DirFilter::kHidden);
    if (!showHidden) {   // hide files
        const QString &parentPath = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardParentPath).toString();
        const QUrl &urlHidden = QUrl::fromLocalFile(parentPath + "/.hidden");

        QSet<QString> hideList;
        if (hideListMap.count(urlHidden) > 0) {
            hideList = hideListMap.value(urlHidden);
        } else {
            hideList = DLocalHelper::hideListFromUrl(urlHidden);
            hideListMap.insert(urlHidden, hideList);
        }
        bool isHidden = DLocalHelper::fileIsHidden(dfileInfoNext.data(), hideList, false);
        if (isHidden)
            ret = false;
    }

    // filter name
    const bool caseSensitive = (dirFilters & DEnumerator::DirFilter::kCaseSensitive).testFlag(DEnumerator::DirFilter::kCaseSensitive);
    if (nameFilters.contains(fileInfoName, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive))
        ret = false;

    const bool showDot = !((dirFilters & DEnumerator::DirFilter::kNoDotAndDotDot).testFlag(DEnumerator::DirFilter::kNoDotAndDotDot))
            && !((dirFilters & DEnumerator::DirFilter::kNoDot).testFlag(DEnumerator::DirFilter::kNoDot));
    const bool showDotDot = !((dirFilters & DEnumerator::DirFilter::kNoDotAndDotDot).testFlag(DEnumerator::DirFilter::kNoDotAndDotDot))
            && !((dirFilters & DEnumerator::DirFilter::kNoDotDot).testFlag(DEnumerator::DirFilter::kNoDotDot));
    if (!showDot && fileInfoName == ".")
        ret = false;
    if (!showDotDot && fileInfoName == "..")
        ret = false;

    return ret;
}

FTS *DEnumeratorPrivate::openDirByfts()
{
    FTS *fts { nullptr };
    QString path = q->uri().path();
    if (path != "/" && path.endsWith("/"))
        path = path.left(path.length() - 1);
    char *paths[2] = { nullptr, nullptr };
    paths[0] = strdup(path.toUtf8().toStdString().data());
    int (*compare)(const FTSENT **, const FTSENT **);
    compare = nullptr;
    if (sortRoleFlag == DEnumerator::SortRoleCompareFlag::kSortRoleCompareFileName) {
        compare = DLocalHelper::compareByName;
    } else if (sortRoleFlag == DEnumerator::SortRoleCompareFlag::kSortRoleCompareFileSize) {
        compare = DLocalHelper::compareBySize;
    } else if (sortRoleFlag == DEnumerator::SortRoleCompareFlag::kSortRoleCompareFileLastModified) {
        compare = DLocalHelper::compareByLastModifed;
    } else if (sortRoleFlag == DEnumerator::SortRoleCompareFlag::kSortRoleCompareFileLastRead) {
        compare = DLocalHelper::compareByLastRead;
    }

    fts = fts_open(paths, FTS_COMFOLLOW, compare);

    if (paths[0])
        free(paths[0]);

    if (nullptr == fts) {
        qWarning() << "fts_open open error : " << QString::fromLocal8Bit(strerror(errno));
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_FTS_OPEN);
        return nullptr;
    }

    return fts;
}

void DEnumeratorPrivate::insertSortFileInfoList(QList<QSharedPointer<DEnumerator::SortFileInfo>> &fileList, QList<QSharedPointer<DEnumerator::SortFileInfo>> &dirList, FTSENT *ent, FTS *fts, const QSet<QString> &hideList)
{
    QSharedPointer<DFileInfo> info(nullptr);
    bool isDir = S_ISDIR(ent->fts_statp->st_mode);
    if (S_ISLNK(ent->fts_statp->st_mode)) {
        const QUrl &url = QUrl::fromLocalFile(ent->fts_path);
        info = DLocalHelper::createFileInfoByUri(url);
        isDir = info->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool();
    }

    if (isDir)
        fts_set(fts, ent, FTS_SKIP);

    if (isDir && !isMixDirAndFile) {
        if (sortOrder == Qt::DescendingOrder)
            dirList.push_front(DLocalHelper::createSortFileInfo(ent, info, hideList));
        else
            dirList.push_back(DLocalHelper::createSortFileInfo(ent, info, hideList));
        return;
    }

    if (sortOrder == Qt::DescendingOrder)
        fileList.push_front(DLocalHelper::createSortFileInfo(ent, info, hideList));
    else
        fileList.push_back(DLocalHelper::createSortFileInfo(ent, info, hideList));
}

void DEnumeratorPrivate::enumUriAsyncOvered(GList *files)
{
    asyncOvered = !files;
    if (!files) {
        asyncIteratorOver();
        return;
    }
    GList *l;
    for (l = files; l != nullptr; l = l->next) {
        asyncInfos.append(static_cast<GFileInfo *>(l->data));
    }
    g_list_free(files);
}

void DEnumeratorPrivate::startAsyncIterator()
{
    qInfo() << "start Async Iterator，uri = " << uri;
    asyncStoped = false;
    const QString &uriPath = uri.toString();
    g_autoptr(GFile) gfile = g_file_new_for_uri(uriPath.toLocal8Bit().data());

    checkAndResetCancel();
    EnumUriData *userData = new EnumUriData();
    userData->pointer = sharedFromThis();
    g_file_enumerate_children_async(gfile,
                                    FILE_DEFAULT_ATTRIBUTES,
                                    G_FILE_QUERY_INFO_NONE,
                                    G_PRIORITY_DEFAULT,
                                    cancellable,
                                    enumUriAsyncCallBack,
                                    userData);
}

bool DEnumeratorPrivate::hasNext()
{
    if (!asyncOvered)
        return false;

    if (asyncInfos.isEmpty())
        return false;

    auto gfileInfo = asyncInfos.takeFirst();

    if (!gfileInfo)
        return hasNext();

    nextUrl = QUrl::fromLocalFile(uri.path() + "/" + QString(g_file_info_get_name(gfileInfo)));

    dfileInfoNext = DLocalHelper::createFileInfoByUri(nextUrl, g_file_info_dup(gfileInfo), FILE_DEFAULT_ATTRIBUTES,
                                                      enumLinks ? DFileInfo::FileQueryInfoFlags::kTypeNone : DFileInfo::FileQueryInfoFlags::kTypeNoFollowSymlinks);

    g_object_unref(gfileInfo);

    if (!checkFilter())
        return hasNext();

    return true;
}

QList<QSharedPointer<DFileInfo>> DEnumeratorPrivate::fileInfoList()
{
    if (asyncOvered)
        return QList<QSharedPointer<DFileInfo>>();
    for (auto gfileInfo : asyncInfos) {
        if (!gfileInfo)
            continue;
        auto url = QUrl::fromLocalFile(uri.path() + "/" + QString(g_file_info_get_name(gfileInfo)));

        infoList.append(DLocalHelper::createFileInfoByUri(url, g_file_info_dup(gfileInfo), FILE_DEFAULT_ATTRIBUTES,
                                                          enumLinks ? DFileInfo::FileQueryInfoFlags::kTypeNone
                                                                    : DFileInfo::FileQueryInfoFlags::kTypeNoFollowSymlinks));
        g_object_unref(gfileInfo);
    }

    return infoList;
}

void DEnumeratorPrivate::enumUriAsyncCallBack(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    EnumUriData *data = static_cast<EnumUriData *>(userData);
    if (!data || !data->pointer || data->pointer->asyncStoped) {
        qInfo() << "user data error " << data;
        return;
    }

    GFileEnumerator *enumerator;
    GError *error { nullptr };
    enumerator = g_file_enumerate_children_finish(G_FILE(sourceObject), res, &error);

    if (error) {
        qInfo() << "enumerator url : " << data->pointer->uri << ". error msg : " << error->message;
        data->pointer->setErrorFromGError(error);
    }

    if (enumerator == nullptr || error) {
        data->pointer->enumUriAsyncOvered(nullptr);
    } else {
        data->enumerator = enumerator;
        data->pointer->checkAndResetCancel();
        g_file_enumerator_next_files_async(enumerator,
                                           1000,
                                           G_PRIORITY_DEFAULT,
                                           data->pointer->cancellable,
                                           moreFilesCallback,
                                           data);
    }

    if (error)
        g_error_free(error);

    return;
}

void DEnumeratorPrivate::moreFilesCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    Q_UNUSED(sourceObject);
    EnumUriData *data = static_cast<EnumUriData *>(userData);
    if (!data || !data->pointer || data->pointer->asyncStoped) {
        qInfo() << "user data error " << data;
        return;
    }

    GError *error;
    GList *files;
    error = nullptr;
    GFileEnumerator *enumerator = data->enumerator;
    files = g_file_enumerator_next_files_finish(enumerator,
                                                res, &error);
    if (error)
        data->pointer->setErrorFromGError(error);

    data->pointer->enumUriAsyncOvered(files);
    if (files && !error) {
        data->pointer->checkAndResetCancel();
        g_file_enumerator_next_files_async(enumerator,
                                           100,
                                           G_PRIORITY_DEFAULT,
                                           data->pointer->cancellable,
                                           moreFilesCallback,
                                           data);
    } else {
        if (!g_file_enumerator_is_closed(data->enumerator)) {
            g_file_enumerator_close_async(data->enumerator,
                                          0, nullptr, nullptr, nullptr);
        }
        g_object_unref(data->enumerator);
        data->enumerator = nullptr;
    }

    if (error)
        g_error_free(error);

}

/************************************************
 * DEnumerator
 ***********************************************/

DEnumerator::DEnumerator(const QUrl &uri)
    : d(new DEnumeratorPrivate(this))
{
    d->uri = uri;
}

DEnumerator::DEnumerator(const QUrl &uri, const QStringList &nameFilters, DirFilters filters, IteratorFlags flags)
    : d(new DEnumeratorPrivate(this))
{
    d->uri = uri;
    d->nameFilters = nameFilters;
    d->dirFilters = filters;
    d->iteratorFlags = flags;

    d->enumSubDir = d->iteratorFlags & DEnumerator::IteratorFlag::kSubdirectories;
    d->enumLinks = d->iteratorFlags & DEnumerator::IteratorFlag::kFollowSymlinks;
}

DEnumerator::~DEnumerator()
{
}

QUrl DEnumerator::uri() const
{
    return d->uri;
}

void DEnumerator::setNameFilters(const QStringList &filters)
{
    d->nameFilters = filters;
}

QStringList DEnumerator::nameFilters() const
{
    return d->nameFilters;
}

void DEnumerator::setDirFilters(DirFilters filters)
{
    d->dirFilters = filters;
}

DEnumerator::DirFilters DEnumerator::dirFilters() const
{
    return d->dirFilters;
}

void DEnumerator::setIteratorFlags(IteratorFlags flags)
{
    d->iteratorFlags = flags;
}

DEnumerator::IteratorFlags DEnumerator::iteratorFlags() const
{
    return d->iteratorFlags;
}

void DEnumerator::setTimeout(ulong timeout)
{
    d->enumTimeout = timeout;
}

ulong DEnumerator::timeout() const
{
    return d->enumTimeout;
}

void DEnumerator::setSortRole(SortRoleCompareFlag role)
{
    d->sortRoleFlag = role;
}

DEnumerator::SortRoleCompareFlag DEnumerator::sortRole() const
{
    return d->sortRoleFlag;
}

void DEnumerator::setSortOrder(Qt::SortOrder order)
{
    d->sortOrder = order;
}

Qt::SortOrder DEnumerator::sortOrder() const
{
    return d->sortOrder;
}

void DEnumerator::setSortMixed(bool mix)
{
    d->isMixDirAndFile = mix;
}

bool DEnumerator::isSortMixed() const
{
    return d->isMixDirAndFile;
}

bool DEnumerator::cancel()
{
    if (d->cancellable && !g_cancellable_is_cancelled(d->cancellable))
        g_cancellable_cancel(d->cancellable);
    d->ftsCanceled = true;
    d->asyncStoped = true;
    return true;
}

bool DEnumerator::hasNext() const
{
    if (d->async)
        return d->hasNext();

    if (!d->inited)
        d->init();

    if (d->stackEnumerator.isEmpty())
        return false;

    // sub dir enumerator
    if (d->enumSubDir && d->dfileInfoNext && d->dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool()) {
        bool showDir = true;
        if (d->dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool()) {
            // is symlink, need enumSymlink
            showDir = d->enumLinks;
        }
        if (showDir)
            d->init(d->nextUrl);
    }
    if (d->stackEnumerator.isEmpty())
        return false;

    GFileEnumerator *enumerator = d->stackEnumerator.top();

    GFileInfo *gfileInfo = nullptr;
    GFile *gfile = nullptr;

    g_autoptr(GError) gerror = nullptr;
    d->checkAndResetCancel();
    bool hasNext = g_file_enumerator_iterate(enumerator, &gfileInfo, &gfile, d->cancellable, &gerror);
    if (hasNext) {
        if (!gfileInfo || !gfile) {
            GFileEnumerator *enumeratorPop = d->stackEnumerator.pop();
            g_object_unref(enumeratorPop);
            return this->hasNext();
        }

        g_autofree gchar *path = g_file_get_path(gfile);
        if (path) {
            d->nextUrl = QUrl::fromLocalFile(QString::fromLocal8Bit(path));
        } else {
            g_autofree gchar *uri = g_file_get_uri(gfile);
            d->nextUrl = QUrl(QString::fromLocal8Bit(uri));
        }
        d->dfileInfoNext = DLocalHelper::createFileInfoByUri(d->nextUrl, g_file_info_dup(gfileInfo), FILE_DEFAULT_ATTRIBUTES,
                                                             d->enumLinks ? DFileInfo::FileQueryInfoFlags::kTypeNone : DFileInfo::FileQueryInfoFlags::kTypeNoFollowSymlinks);

        if (!d->checkFilter())
            return this->hasNext();

        return true;
    }

    if (gerror)
        d->setErrorFromGError(gerror);

    return false;
}

QUrl DEnumerator::next() const
{
    return d->nextUrl;
}

QSharedPointer<DFileInfo> DEnumerator::fileInfo() const
{
    return d->dfileInfoNext;
}

quint64 DEnumerator::fileCount()
{
    if (!d->inited)
        d->init();

    quint64 count = 0;

    while (hasNext())
        ++count;

    return count;
}

QList<QSharedPointer<DFileInfo>> DEnumerator::fileInfoList()
{
    if (d->async)
        return d->fileInfoList();

    g_autoptr(GFileEnumerator) enumerator = nullptr;
    g_autoptr(GError) gerror = nullptr;

    g_autoptr(GFile) gfile = g_file_new_for_uri(d->uri.toString().toStdString().c_str());

    d->checkAndResetCancel();
    enumerator = g_file_enumerate_children(gfile,
                                           FILE_DEFAULT_ATTRIBUTES,
                                           d->enumLinks ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                           d->cancellable,
                                           &gerror);

    if (nullptr == enumerator) {
        if (gerror) {
            d->setErrorFromGError(gerror);
        }
        return d->infoList;
    }

    GFile *gfileIn = nullptr;
    GFileInfo *gfileInfoIn = nullptr;

    d->checkAndResetCancel();
    while (g_file_enumerator_iterate(enumerator, &gfileInfoIn, &gfileIn, d->cancellable, &gerror)) {
        if (!gfileInfoIn)
            break;

        g_autofree gchar *uri = g_file_get_uri(gfileIn);
        const QUrl &url = QUrl(QString::fromLocal8Bit(uri));
        QSharedPointer<DFileInfo> info = DLocalHelper::createFileInfoByUri(url);
        if (info)
            d->infoList.append(info);

        if (gerror) {
            d->setErrorFromGError(gerror);
            gerror = nullptr;
        }
    }

    if (gerror)
        d->setErrorFromGError(gerror);

    return d->infoList;
}

QList<QSharedPointer<DEnumerator::SortFileInfo>> DEnumerator::sortFileInfoList()
{
    FTS *fts = d->openDirByfts();
    if (!fts)
        return {};

    QList<QSharedPointer<DEnumerator::SortFileInfo>> listFile;
    QList<QSharedPointer<DEnumerator::SortFileInfo>> listDir;
    QSet<QString> hideList;
    const QUrl &urlHidden = QUrl::fromLocalFile(d->uri.path() + "/.hidden");
    hideList = DLocalHelper::hideListFromUrl(urlHidden);
    while (1) {
        FTSENT *ent = fts_read(fts);

        if (ent == nullptr) {
            break;
        }

        if (d->ftsCanceled)
            break;

        unsigned short flag = ent->fts_info;

        if (QString(ent->fts_path) == d->uri.path() || flag == FTS_DP)
            continue;

        d->insertSortFileInfoList(listFile, listDir, ent, fts, hideList);
    }

    fts_close(fts);
    fts = nullptr;

    if (d->isMixDirAndFile)
        return listFile;

    listDir.append(listFile);
    return listDir;
}

DFMIOError DEnumerator::lastError() const
{
    return d->error;
}

DEnumeratorFuture *DEnumerator::asyncIterator()
{
    d->async = true;
    DEnumeratorFuture *future = new DEnumeratorFuture(sharedFromThis());
    QObject::connect(d.data(), &DEnumeratorPrivate::asyncIteratorOver, future, &DEnumeratorFuture::onAsyncIteratorOver);
    return future;
}

void DEnumerator::startAsyncIterator()
{
    d->startAsyncIterator();
}

bool DEnumerator::isAsyncOver() const
{
    return d->asyncOvered;
}
