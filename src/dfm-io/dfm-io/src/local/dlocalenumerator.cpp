/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     dengkeyun<dengkeyun@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *             zhangsheng<zhangsheng@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
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

#include "local/dlocalenumerator_p.h"
#include "local/dlocalenumerator.h"
#include "local/dlocalhelper.h"

#include "core/dfileinfo.h"

#include <gio/gio.h>

#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>

#define FILE_DEFAULT_ATTRIBUTES "standard::*,etag::*,id::*,access::*,mountable::*,time::*,unix::*,dos::*,\
owner::*,thumbnail::*,preview::*,filesystem::*,gvfs::*,selinux::*,trash::*,recent::*,metadata::*"

USING_IO_NAMESPACE

DLocalEnumeratorPrivate::DLocalEnumeratorPrivate(DLocalEnumerator *q)
    : q(q)
{
}

DLocalEnumeratorPrivate::~DLocalEnumeratorPrivate()
{
    clean();
}

QList<QSharedPointer<DFileInfo>> DLocalEnumeratorPrivate::fileInfoList()
{
    g_autoptr(GFileEnumerator) enumerator = nullptr;
    g_autoptr(GError) gerror = nullptr;

    g_autoptr(GFile) gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());

    enumerator = g_file_enumerate_children(gfile,
                                           FILE_DEFAULT_ATTRIBUTES,
                                           enumLinks ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                           nullptr,
                                           &gerror);

    if (nullptr == enumerator) {
        if (gerror) {
            setErrorFromGError(gerror);
        }
        return list_;
    }

    GFile *gfileIn = nullptr;
    GFileInfo *gfileInfoIn = nullptr;

    while (g_file_enumerator_iterate(enumerator, &gfileInfoIn, &gfileIn, nullptr, &gerror)) {
        if (!gfileInfoIn)
            break;

        g_autofree gchar *uri = g_file_get_uri(gfileIn);
        const QUrl &url = QUrl(QString::fromLocal8Bit(uri));
        QSharedPointer<DFileInfo> info = DLocalHelper::createFileInfoByUri(url);
        if (info)
            list_.append(info);

        if (gerror) {
            setErrorFromGError(gerror);
            gerror = nullptr;
        }
    }

    if (gerror) {
        setErrorFromGError(gerror);
    }

    return list_;
}

bool DLocalEnumeratorPrivate::hasNext()
{
    if (stackEnumerator.isEmpty())
        return false;

    // sub dir enumerator
    if (enumSubDir && dfileInfoNext && dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool()) {
        bool showDir = true;
        if (dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool()) {
            // is symlink, need enumSymlink
            showDir = enumLinks;
        }
        if (showDir) {
            init(nextUrl);
        }
    }
    if (stackEnumerator.isEmpty())
        return false;

    GFileEnumerator *enumerator = stackEnumerator.top();

    GFileInfo *gfileInfo = nullptr;
    GFile *gfile = nullptr;

    g_autoptr(GError) gerror = nullptr;
    bool hasNext = g_file_enumerator_iterate(enumerator, &gfileInfo, &gfile, nullptr, &gerror);
    if (hasNext) {
        if (!gfileInfo || !gfile) {
            GFileEnumerator *enumeratorPop = stackEnumerator.pop();
            g_object_unref(enumeratorPop);
            return this->hasNext();
        }

        g_autofree gchar *path = g_file_get_path(gfile);
        if (path) {
            nextUrl = QUrl::fromLocalFile(QString::fromLocal8Bit(path));
        } else {
            g_autofree gchar *uri = g_file_get_uri(gfile);
            nextUrl = QUrl(QString::fromLocal8Bit(uri));
        }
        dfileInfoNext = DLocalHelper::createFileInfoByUri(nextUrl, g_file_info_dup(gfileInfo), FILE_DEFAULT_ATTRIBUTES,
                                                          enumLinks ? DFileInfo::FileQueryInfoFlags::kTypeNone : DFileInfo::FileQueryInfoFlags::kTypeNoFollowSymlinks);

        if (!checkFilter())
            return this->hasNext();

        return true;
    }

    if (gerror) {
        setErrorFromGError(gerror);
    }
    return false;
}

QUrl DLocalEnumeratorPrivate::next() const
{
    return nextUrl;
}

QSharedPointer<DFileInfo> DLocalEnumeratorPrivate::fileInfo() const
{
    return dfileInfoNext;
}

quint64 DLocalEnumeratorPrivate::fileCount()
{
    quint64 count = 0;

    while (hasNext()) {
        ++count;
    }

    return count;
}

bool DLocalEnumeratorPrivate::checkFilter()
{
    if (dirFilters == kDirFilterNofilter)
        return true;

    if (!dfileInfoNext)
        return false;

    const bool isDir = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsDir).toBool();
    if ((dirFilters & DEnumerator::DirFilter::kAllDirs) == kDirFilterAllDirs) {   // all dir, no apply filters rules
        if (isDir)
            return true;
    }

    // dir filter
    bool ret = true;

    const bool readable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanRead).toBool();
    const bool writable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanWrite).toBool();
    const bool executable = dfileInfoNext->attribute(DFileInfo::AttributeID::kAccessCanExecute).toBool();

    auto checkRWE = [&]() -> bool {
        if ((dirFilters & DEnumerator::DirFilter::kReadable) == kDirFilterReadable) {
            if (!readable)
                return false;
        }
        if ((dirFilters & DEnumerator::DirFilter::kWritable) == kDirFilterWritable) {
            if (!writable)
                return false;
        }
        if ((dirFilters & DEnumerator::DirFilter::kExecutable) == kDirFilterExecutable) {
            if (!executable)
                return false;
        }
        return true;
    };

    if ((dirFilters & DEnumerator::DirFilter::kAllEntries) == kDirFilterAllEntries
        || ((dirFilters & DEnumerator::DirFilter::kDirs) && (dirFilters & DEnumerator::DirFilter::kFiles))) {
        // 判断读写执行
        if (!checkRWE())
            ret = false;
    } else if ((dirFilters & DEnumerator::DirFilter::kDirs) == kDirFilterDirs) {
        if (!isDir) {
            ret = false;
        } else {
            // 判断读写执行
            if (!checkRWE())
                ret = false;
        }
    } else if ((dirFilters & DEnumerator::DirFilter::kFiles) == kDirFilterFiles) {
        const bool isFile = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsFile).toBool();
        if (!isFile) {
            ret = false;
        } else {
            // 判断读写执行
            if (!checkRWE())
                ret = false;
        }
    }

    if ((dirFilters & DEnumerator::DirFilter::kNoSymLinks) == kDirFilterNoSymLinks) {
        const bool isSymlinks = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardIsSymlink).toBool();
        if (isSymlinks)
            ret = false;
    }

    const QString &fileInfoName = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardName).toString();
    const bool showHidden = (dirFilters & DEnumerator::DirFilter::kHidden) == kDirFilterHidden;
    if (!showHidden) {   // hide files
        const QString &parentPath = dfileInfoNext->attribute(DFileInfo::AttributeID::kStandardParentPath).toString();
        const QUrl &urlHidden = QUrl::fromLocalFile(parentPath + "/.hidden");

        QSet<QString> hideList;
        if (hideListMap.count(urlHidden) > 0) {
            hideList = hideListMap.value(urlHidden);
        } else {
            hideList = DLocalHelper::hideListFromUrl(urlHidden);
            if (!hideList.empty())
                hideListMap.insert(urlHidden, hideList);
        }
        bool isHidden = DLocalHelper::fileIsHidden(dfileInfoNext, hideList);
        if (isHidden)
            ret = false;
    }

    // filter name
    const bool caseSensitive = (dirFilters & DEnumerator::DirFilter::kCaseSensitive) == kDirFilterCaseSensitive;
    if (nameFilters.contains(fileInfoName, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive))
        ret = false;

    const bool showDot = !((dirFilters & DEnumerator::DirFilter::kNoDotAndDotDot) == kDirFilterNoDotAndDotDot) && !((dirFilters & DEnumerator::DirFilter::kNoDot) == kDirFilterNoDot);
    const bool showDotDot = !((dirFilters & DEnumerator::DirFilter::kNoDotAndDotDot) == kDirFilterNoDotAndDotDot) && !((dirFilters & DEnumerator::DirFilter::kNoDotDot) == kDirFilterNoDotDot);
    if (!showDot && fileInfoName == ".")
        ret = false;
    if (!showDotDot && fileInfoName == "..")
        ret = false;

    return ret;
}

DFMIOError DLocalEnumeratorPrivate::lastError()
{
    return error;
}

void DLocalEnumeratorPrivate::init(const QUrl &url)
{
    QPointer<DLocalEnumeratorPrivate> me = this;
    const bool needTimeOut = q->timeout() != 0;
    if (!needTimeOut) {
        createEnumerator(url, me);
    } else {
        mutex.lock();
        QtConcurrent::run([this, me, url]() {
            createEnumerator(url, me);
        });
        bool succ = waitCondition.wait(&mutex, q->timeout());
        mutex.unlock();
        if (!succ)
            qWarning() << "createEnumeratorInThread failed, url: " << url;
    }
}

void DLocalEnumeratorPrivate::setErrorFromGError(GError *gerror)
{
    error.setCode(DFMIOErrorCode(gerror->code));
}

void DLocalEnumeratorPrivate::clean()
{
    if (!stackEnumerator.isEmpty()) {
        while (1) {
            GFileEnumerator *enumerator = stackEnumerator.pop();
            g_object_unref(enumerator);
            if (stackEnumerator.isEmpty())
                break;
        }
    }
}

void DLocalEnumeratorPrivate::createEnumeratorInThread(const QUrl &url)
{
}

void DLocalEnumeratorPrivate::createEnumerator(const QUrl &url, QPointer<DLocalEnumeratorPrivate> me)
{
    const QString &uriPath = url.toString();

    g_autoptr(GFile) gfile = g_file_new_for_uri(uriPath.toLocal8Bit().data());

    g_autoptr(GError) gerror = nullptr;
    GFileEnumerator *genumerator = g_file_enumerate_children(gfile,
                                                             FILE_DEFAULT_ATTRIBUTES,
                                                             enumLinks ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                             nullptr,
                                                             &gerror);
    if (!me)
        return;
    if (!genumerator || gerror) {
        if (gerror) {
            setErrorFromGError(gerror);
        }
    } else {
        stackEnumerator.push_back(genumerator);
    }
    waitCondition.wakeAll();
}

DLocalEnumerator::DLocalEnumerator(const QUrl &uri, const QStringList &nameFilters, DirFilters filters, IteratorFlags flags)
    : DEnumerator(uri, nameFilters, filters, flags), d(new DLocalEnumeratorPrivate(this))
{
    registerFileInfoList(std::bind(&DLocalEnumerator::fileInfoList, this));
    registerHasNext(std::bind(&DLocalEnumerator::hasNext, this));
    registerNext(std::bind(&DLocalEnumerator::next, this));
    registerFileInfo(std::bind(&DLocalEnumerator::fileInfo, this));
    registerFileCount(std::bind(&DLocalEnumerator::fileCount, this));
    registerLastError(std::bind(&DLocalEnumerator::lastError, this));

    d->nameFilters = nameFilters;
    d->dirFilters = filters;
    d->iteratorFlags = flags;

    d->enumSubDir = d->iteratorFlags & DEnumerator::IteratorFlag::kSubdirectories;
    d->enumLinks = d->iteratorFlags & DEnumerator::IteratorFlag::kFollowSymlinks;

    d->init(uri);
}

DLocalEnumerator::~DLocalEnumerator()
{
}

bool DLocalEnumerator::hasNext() const
{
    return d->hasNext();
}

QUrl DLocalEnumerator::next() const
{
    return d->next();
}

QSharedPointer<DFileInfo> DLocalEnumerator::fileInfo() const
{
    return d->fileInfo();
}

quint64 DLocalEnumerator::fileCount()
{
    return d->fileCount();
}

DFMIOError DLocalEnumerator::lastError() const
{
    return d->lastError();
}

QList<QSharedPointer<DFileInfo>> DLocalEnumerator::fileInfoList()
{
    return d->fileInfoList();
}
