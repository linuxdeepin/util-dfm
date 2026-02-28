// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/denumerator_p.h"

#include "utils/dlocalhelper.h"

#include <dfm-io/denumerator.h>
#include <dfm-io/dfileinfo.h>
#include <dfm-io/denumeratorfuture.h>
#include <dfm-io/dfmio_utils.h>

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
    queryAttributes = FILE_DEFAULT_ATTRIBUTES;
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
                                                             queryAttributes.toStdString().c_str(),
                                                             enumLinks ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                             cancellable,
                                                             &gerror);
    if (!me) {
        // Clean up the enumerator if it was created but the object is no longer valid
        if (genumerator) {
            g_object_unref(genumerator);
        }
        error.setCode(DFMIOErrorCode(DFM_IO_ERROR_NOT_FOUND));
        return false;
    }
    bool ret = true;
    if (!genumerator || gerror) {
        // Clean up the enumerator if it was created but the object is no longer valid
        if (genumerator) {
            g_object_unref(genumerator);
        }
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
        if (!g_cancellable_is_cancelled(cancellable))
            g_cancellable_cancel(cancellable);
        g_cancellable_reset(cancellable);
        return;
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

    // 1. 首先处理特殊目录 "." 和 ".."
    const QString &fileName = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardName).toString();
    if (!shouldShowDotAndDotDot(fileName))
        return false;

    // 2. 处理目录和文件的过滤
    if (!checkEntryTypeFilter())
        return false;

    // 3. 处理权限过滤
    if (!checkPermissionFilter())
        return false;

    // 4. 处理符号链接过滤
    if (!checkSymlinkFilter())
        return false;

    // 5. 处理隐藏文件过滤
    if (!checkHiddenFilter())
        return false;

    // 6. 处理文件名过滤器
    if (!checkNameFilter(fileName))
        return false;

    return true;
}

bool DEnumeratorPrivate::shouldShowDotAndDotDot(const QString &fileName)
{
    const bool isDot = (fileName == ".");
    const bool isDotDot = (fileName == "..");

    if (isDot && (dirFilters.testFlag(DEnumerator::DirFilter::kNoDot) || dirFilters.testFlag(DEnumerator::DirFilter::kNoDotAndDotDot)))
        return false;

    if (isDotDot && (dirFilters.testFlag(DEnumerator::DirFilter::kNoDotDot) || dirFilters.testFlag(DEnumerator::DirFilter::kNoDotAndDotDot)))
        return false;

    return true;
}

bool DEnumeratorPrivate::checkEntryTypeFilter()
{
    const bool isDir = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool();
    const bool isSymlink = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool();
    
    // kAllDirs: 显示所有目录，包括符号链接指向的目录
    if (dirFilters.testFlag(DEnumerator::DirFilter::kAllDirs)) {
        if (isDir)
            return true;
            
        if (isSymlink) {
            // 对于符号链接，需要检查其目标
            const QString targetPath = dfileInfoNext->attribute(
                DFileInfo::AttributeID::kStandardSymlinkTarget).toString();
            if (!targetPath.isEmpty()) {
                // 如果是相对路径，需要转换为绝对路径
                QString absoluteTargetPath = targetPath;
                if (QDir::isRelativePath(targetPath)) {
                    const QString parentPath = dfileInfoNext->attribute(
                        DFileInfo::AttributeID::kStandardParentPath).toString();
                    absoluteTargetPath = QDir(parentPath).absoluteFilePath(targetPath);
                }
                
                QUrl targetUrl = QUrl::fromLocalFile(absoluteTargetPath);
                QSharedPointer<DFileInfo> targetInfo = DLocalHelper::createFileInfoByUri(targetUrl);
                if (targetInfo) {
                    return targetInfo->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool();
                }
            }
        }
        return false;
    }
    
    // 如果没有指定 kAllDirs，则根据 kDirs 和 kFiles 过滤
    const bool wantDirs = dirFilters.testFlag(DEnumerator::DirFilter::kDirs);
    const bool wantFiles = dirFilters.testFlag(DEnumerator::DirFilter::kFiles);
    const bool isFile = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsFile).toBool();
    
    // 如果既不要目录也不要文件，返回 false
    if (!wantDirs && !wantFiles)
        return false;
        
    // 如果只要目录
    if (wantDirs && !wantFiles)
        return isDir;
        
    // 如果只要文件
    if (!wantDirs && wantFiles)
        return isFile;
        
    // 如果都要
    return true;
}

bool DEnumeratorPrivate::checkPermissionFilter()
{
    if (!dirFilters.testFlag(DEnumerator::DirFilter::kReadable) && !dirFilters.testFlag(DEnumerator::DirFilter::kWritable) && !dirFilters.testFlag(DEnumerator::DirFilter::kExecutable))
        return true;

    const bool readable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanRead).toBool();
    const bool writable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanWrite).toBool();
    const bool executable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanExecute).toBool();

    if (dirFilters.testFlag(DEnumerator::DirFilter::kReadable) && !readable)
        return false;
    if (dirFilters.testFlag(DEnumerator::DirFilter::kWritable) && !writable)
        return false;
    if (dirFilters.testFlag(DEnumerator::DirFilter::kExecutable) && !executable)
        return false;

    return true;
}

bool DEnumeratorPrivate::checkSymlinkFilter()
{
    if (!dirFilters.testFlag(DEnumerator::DirFilter::kNoSymLinks))
        return true;

    return !dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool();
}

bool DEnumeratorPrivate::checkHiddenFilter()
{
    if (dirFilters.testFlag(DEnumerator::DirFilter::kHidden))
        return true;

    const QString &parentPath = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardParentPath).toString();
    const QUrl &urlHidden = QUrl::fromLocalFile(parentPath + "/.hidden");

    QSet<QString> hideList;
    if (hideListMap.count(urlHidden) > 0) {
        hideList = hideListMap.value(urlHidden);
    } else {
        hideList = DLocalHelper::hideListFromUrl(urlHidden);
        hideListMap.insert(urlHidden, hideList);
    }

    return !DLocalHelper::fileIsHidden(dfileInfoNext.data(), hideList, false);
}

bool DEnumeratorPrivate::checkNameFilter(const QString &fileName)
{
    if (nameFilters.isEmpty())
        return true;

    const bool caseSensitive = dirFilters.testFlag(DEnumerator::DirFilter::kCaseSensitive);
    return !nameFilters.contains(fileName, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

bool DEnumeratorPrivate::openDirByfts()
{
    char *paths[2] = { nullptr, nullptr };
    paths[0] = filePath(uri);
    if (!paths[0]) {
        error.setCode(DFMIOErrorCode::DFM_IO_ERROR_FAILED);
        return false;
    }
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
        return false;
    }

    return true;
}

void DEnumeratorPrivate::insertSortFileInfoList(QList<QSharedPointer<DEnumerator::SortFileInfo>> &fileList, QList<QSharedPointer<DEnumerator::SortFileInfo>> &dirList, FTSENT *ent, FTS *fts, const QSet<QString> &hideList)
{
    auto sortInfo = DLocalHelper::createSortFileInfo(ent, hideList);
    if (sortInfo->isDir && !sortInfo->isSymLink)
        fts_set(fts, ent, FTS_SKIP);

    if (sortInfo->isDir && !isMixDirAndFile) {
        if (sortOrder == Qt::DescendingOrder)
            dirList.push_front(sortInfo);
        else
            dirList.push_back(sortInfo);
        return;
    }

    if (sortOrder == Qt::DescendingOrder)
        fileList.push_front(sortInfo);
    else
        fileList.push_back(sortInfo);
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
    g_autoptr(GFile) gfile = DLocalHelper::createGFile(uri);

    checkAndResetCancel();
    EnumUriData *userData = new EnumUriData();
    userData->pointer = sharedFromThis();
    g_file_enumerate_children_async(gfile,
                                    queryAttributes.toStdString().c_str(),
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

    while (!asyncInfos.isEmpty()) {
        auto gfileInfo = asyncInfos.takeFirst();

        if (!gfileInfo)
            continue;

        nextUrl = buildUrl(uri, g_file_info_get_name(gfileInfo));

        dfileInfoNext = DLocalHelper::createFileInfoByUri(nextUrl, g_file_info_dup(gfileInfo), queryAttributes.toStdString().c_str(),
                                                          enumLinks ? DFileInfo::FileQueryInfoFlags::kTypeNone : DFileInfo::FileQueryInfoFlags::kTypeNoFollowSymlinks);

        g_object_unref(gfileInfo);

        if (checkFilter())
            return true;
    }

    return false;
}

QList<QSharedPointer<DFileInfo>> DEnumeratorPrivate::fileInfoList()
{
    if (asyncOvered)
        return QList<QSharedPointer<DFileInfo>>();
    for (auto gfileInfo : asyncInfos) {
        if (!gfileInfo)
            continue;
        auto url = buildUrl(uri, g_file_info_get_name(gfileInfo));

        infoList.append(DLocalHelper::createFileInfoByUri(url, g_file_info_dup(gfileInfo), queryAttributes.toStdString().c_str(),
                                                          enumLinks ? DFileInfo::FileQueryInfoFlags::kTypeNone
                                                                    : DFileInfo::FileQueryInfoFlags::kTypeNoFollowSymlinks));
        g_object_unref(gfileInfo);
    }

    return infoList;
}

void DEnumeratorPrivate::setQueryAttributes(const QString &attributes)
{
    queryAttributes = attributes;
}

char *DEnumeratorPrivate::filePath(const QUrl &url)
{
    if (url.userInfo().startsWith("originPath::"))
        return strdup(url.userInfo().replace("originPath::", "").toLatin1().constData());

    QString path = url.path();
    if (path != "/" && path.endsWith("/"))
        path = path.left(path.length() - 1);
    return strdup(path.toUtf8().constData());
}

QUrl DEnumeratorPrivate::buildUrl(const QUrl &url, const char *fileName)
{
    auto path = url.path()  == "/" ?
                "/" + QString(fileName) :
                url.path() + "/" + QString(fileName);
    QUrl nextUrl = QUrl::fromLocalFile(path);

    if (url.userInfo().startsWith("originPath::")) {
        nextUrl.setUserInfo(url.userInfo() + QString::fromLatin1("/") + QString::fromLatin1(fileName));
    } else if (DFMUtils::isInvalidCodecByPath(fileName)) {
        auto org = url.path()  == "/" ?
                   QString::fromLatin1("/") + QString::fromLatin1(fileName) :
                   QString::fromLatin1(url.path().toUtf8()) + QString::fromLatin1("/") +
                   QString::fromLatin1(fileName);
        nextUrl.setUserInfo(QString::fromLatin1("originPath::") + org);
    }

    return nextUrl;
}

void DEnumeratorPrivate::enumUriAsyncCallBack(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    EnumUriData *data = static_cast<EnumUriData *>(userData);
    if (!data || !data->pointer || data->pointer->asyncStoped) {
        qInfo() << "user data error " << data;
        delete data;  // 修复内存泄漏：在错误情况下删除 data
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
        delete data;  // 修复内存泄漏：在错误情况下删除 data
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
        delete data;  // 修复内存泄漏：在错误情况下删除 data
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
        delete data;  // 修复内存泄漏：在异步操作结束时删除 data
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

void DEnumerator::setQueryAttributes(const QString &attributes)
{
    return d->setQueryAttributes(attributes);
}

QString DEnumerator::queryAttributes() const
{
    return d->queryAttributes;
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

    while (!d->stackEnumerator.isEmpty()) {
        GFileEnumerator *enumerator = d->stackEnumerator.top();
        GFileInfo *gfileInfo = nullptr;
        GFile *gfile = nullptr;

        g_autoptr(GError) gerror = nullptr;
        d->checkAndResetCancel();
        bool hasNext = g_file_enumerator_iterate(enumerator, &gfileInfo, &gfile, d->cancellable, &gerror);

        if (hasNext) {
            if (!gfileInfo || !gfile) {
                // 当前枚举器已完成，弹出并继续下一个
                GFileEnumerator *enumeratorPop = d->stackEnumerator.pop();
                g_object_unref(enumeratorPop);
                continue;
            }

            g_autofree gchar *path = g_file_get_path(gfile);
            if (path) {
                d->nextUrl = QUrl::fromLocalFile(QString::fromLocal8Bit(path));
                if (DFMUtils::isInvalidCodecByPath(path))
                    d->nextUrl.setUserInfo(QString::fromLatin1("originPath::") + QString::fromLatin1(path));
            } else {
                g_autofree gchar *uri = g_file_get_uri(gfile);
                d->nextUrl = QUrl(QString::fromLocal8Bit(uri));
                if (DFMUtils::isInvalidCodecByPath(uri))
                    d->nextUrl.setUserInfo(QString::fromLatin1("originPath::") + QString::fromLatin1(path));
            }
            d->dfileInfoNext = DLocalHelper::createFileInfoByUri(d->nextUrl, g_file_info_dup(gfileInfo), FILE_DEFAULT_ATTRIBUTES,
                                                                 d->enumLinks ? DFileInfo::FileQueryInfoFlags::kTypeNone : DFileInfo::FileQueryInfoFlags::kTypeNoFollowSymlinks);

            // 如果是目录且需要遍历子目录
            if (d->enumSubDir && d->dfileInfoNext && d->dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool()) {
                bool showDir = true;
                if (d->dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool()) {
                    showDir = d->enumLinks;
                }
                if (showDir) {
                    d->init(d->nextUrl);
                }
            }

            if (!d->checkFilter())
                continue;

            return true;
        }

        if (gerror) {
            d->setErrorFromGError(gerror);
            return false;
        }

        // 当前枚举器已完成，弹出并继续下一个
        GFileEnumerator *enumeratorPop = d->stackEnumerator.pop();
        g_object_unref(enumeratorPop);
    }

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

    g_autoptr(GFile) gfile = DLocalHelper::createGFile(d->uri);

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
    if (!d->fts)
        d->openDirByfts();

    if (!d->fts)
        return {};

    QList<QSharedPointer<DEnumerator::SortFileInfo>> listFile;
    QList<QSharedPointer<DEnumerator::SortFileInfo>> listDir;
    QSet<QString> hideList;
    QUrl urlHidden = d->buildUrl(d->uri, ".hidden");
    hideList = DLocalHelper::hideListFromUrl(urlHidden);
    char *dirPath = d->filePath(d->uri);
    if (!dirPath) {
        qWarning() << "Failed to get file path for uri:" << d->uri;
        return {};
    }
    while (1) {
        FTSENT *ent = fts_read(d->fts);

        if (ent == nullptr) {
            break;
        }

        if (d->ftsCanceled)
            break;

        unsigned short flag = ent->fts_info;

        if (strcmp(ent->fts_path, dirPath) == 0 || flag == FTS_DP)
            continue;

        d->insertSortFileInfoList(listFile, listDir, ent, d->fts, hideList);
    }

    fts_close(d->fts);
    d->fts = nullptr;

    // Clean up allocated memory
    free(dirPath);

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

bool DEnumerator::initEnumerator(const bool oneByone)
{
    if (d->async)
        return true;

    if (oneByone) {
        if (d->inited)
            return true;
        return d->init();
    }
    if (d->fts)
        return true;
    return d->openDirByfts();
}
