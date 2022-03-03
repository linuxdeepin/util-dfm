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

#include <QDebug>

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
                                           "*",
                                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
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
        QSharedPointer<DFileInfo> info = DLocalHelper::getFileInfoByUri(QByteArray::fromPercentEncoding(uri));
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
    if (enumSubDir && dfileInfoNext && gfileNext && dfileInfoNext->attribute(DFileInfo::AttributeID::StandardIsDir).toBool()) {
        bool showDir = true;
        if (dfileInfoNext->attribute(DFileInfo::AttributeID::StandardIsSymlink).toBool()) {
            // is symlink, need enumSymlink
            showDir = enumLinks;
        }
        if (showDir) {
            g_autoptr(GError) gerror = nullptr;
            GFileEnumerator *genumerator = g_file_enumerate_children(gfileNext,
                                                                     "*",
                                                                     G_FILE_QUERY_INFO_NONE,
                                                                     nullptr,
                                                                     &gerror);
            if (nullptr == genumerator) {
                if (gerror) {
                    setErrorFromGError(gerror);
                }
            } else {
                stackEnumerator.push_back(genumerator);
            }
        }
    }

    GFileEnumerator *enumerator = stackEnumerator.top();

    g_autoptr(GError) gerror = nullptr;
    GFileInfo *gfileInfo = nullptr;

    bool hasNext = g_file_enumerator_iterate(enumerator, &gfileInfo, &gfileNext, nullptr, &gerror);
    if (hasNext) {

        if (!gfileInfo) {
            GFileEnumerator *enumeratorPop = stackEnumerator.pop();
            g_object_unref(enumeratorPop);
            return this->hasNext();
        }

        g_autofree gchar *uri = g_file_get_uri(gfileNext);
        dfileInfoNext = DLocalHelper::getFileInfoByUri(QByteArray::fromPercentEncoding(uri));
        if (!checkFilter())
            return this->hasNext();

        return true;
    }

    if (gerror) {
        setErrorFromGError(gerror);
    }
    return false;
}

QString DLocalEnumeratorPrivate::next() const
{
    g_autofree char *gpath = g_file_get_path(gfileNext);
    return QString::fromLocal8Bit(gpath);
}

QSharedPointer<DFileInfo> DLocalEnumeratorPrivate::fileInfo() const
{
    return dfileInfoNext;
}

bool DLocalEnumeratorPrivate::checkFilter()
{
    if (dirFilters == kDirFilterNofilter)
        return true;

    if (!dfileInfoNext)
        return false;

    const bool isDir = dfileInfoNext->attribute(DFileInfo::AttributeID::StandardIsDir).toBool();
    if ((dirFilters & DEnumerator::DirFilter::AllDirs) == kDirFilterAllDirs) {   // all dir, no apply filters rules
        if (isDir)
            return true;
    }

    // dir filter
    bool ret = true;

    const bool readable = dfileInfoNext->attribute(DFileInfo::AttributeID::AccessCanRead).toBool();
    const bool writable = dfileInfoNext->attribute(DFileInfo::AttributeID::AccessCanWrite).toBool();
    const bool executable = dfileInfoNext->attribute(DFileInfo::AttributeID::AccessCanExecute).toBool();

    auto checkRWE = [&]() -> bool {
        if ((dirFilters & DEnumerator::DirFilter::Readable) == kDirFilterReadable) {
            if (!readable)
                return false;
        }
        if ((dirFilters & DEnumerator::DirFilter::Writable) == kDirFilterWritable) {
            if (!writable)
                return false;
        }
        if ((dirFilters & DEnumerator::DirFilter::Executable) == kDirFilterExecutable) {
            if (!executable)
                return false;
        }
        return true;
    };

    if ((dirFilters & DEnumerator::DirFilter::AllEntries) == kDirFilterAllEntries
        || ((dirFilters & DEnumerator::DirFilter::Dirs) && (dirFilters & DEnumerator::DirFilter::Files))) {
        // 判断读写执行
        if (!checkRWE())
            ret = false;
    } else if ((dirFilters & DEnumerator::DirFilter::Dirs) == kDirFilterDirs) {
        if (!isDir) {
            ret = false;
        } else {
            // 判断读写执行
            if (!checkRWE())
                ret = false;
        }
    } else if ((dirFilters & DEnumerator::DirFilter::Files) == kDirFilterFiles) {
        const bool isFile = dfileInfoNext->attribute(DFileInfo::AttributeID::StandardIsFile).toBool();
        if (!isFile) {
            ret = false;
        } else {
            // 判断读写执行
            if (!checkRWE())
                ret = false;
        }
    }

    if ((dirFilters & DEnumerator::DirFilter::NoSymLinks) == kDirFilterNoSymLinks) {
        const bool isSymlinks = dfileInfoNext->attribute(DFileInfo::AttributeID::StandardIsSymlink).toBool();
        if (isSymlinks)
            ret = false;
    }

    const QString &fileInfoName = dfileInfoNext->attribute(DFileInfo::AttributeID::StandardName).toString();
    const bool showHidden = (dirFilters & DEnumerator::DirFilter::Hidden) == kDirFilterHidden;
    if (!showHidden) {   // hide files
        bool isHidden = dfileInfoNext->attribute(DFileInfo::AttributeID::StandardIsHidden).toBool();
        if (isHidden)
            ret = false;
    }

    // filter name
    const bool caseSensitive = (dirFilters & DEnumerator::DirFilter::CaseSensitive) == kDirFilterCaseSensitive;
    if (nameFilters.contains(fileInfoName, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive))
        ret = false;

    const bool showDot = !((dirFilters & DEnumerator::DirFilter::NoDotAndDotDot) == kDirFilterNoDotAndDotDot) && !((dirFilters & DEnumerator::DirFilter::NoDot) == kDirFilterNoDot);
    const bool showDotDot = !((dirFilters & DEnumerator::DirFilter::NoDotAndDotDot) == kDirFilterNoDotAndDotDot) && !((dirFilters & DEnumerator::DirFilter::NoDotDot) == kDirFilterNoDotDot);
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

void DLocalEnumeratorPrivate::init()
{
    const QString &&uriPath = q->uri().toString();

    g_autoptr(GError) gerror = nullptr;

    g_autoptr(GFile) gfile = g_file_new_for_uri(uriPath.toLocal8Bit().data());

    GFileEnumerator *genumerator = g_file_enumerate_children(gfile,
                                                             "*",
                                                             G_FILE_QUERY_INFO_NONE,
                                                             nullptr,
                                                             &gerror);

    if (nullptr == genumerator) {
        if (gerror) {
            setErrorFromGError(gerror);
        }
    } else {
        stackEnumerator.push_back(genumerator);
    }
}

void DLocalEnumeratorPrivate::setErrorFromGError(GError *gerror)
{
    error.setCode(DFMIOErrorCode(gerror->code));

    //qWarning() << QString::fromLocal8Bit(gerror->message);
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

DLocalEnumerator::DLocalEnumerator(const QUrl &uri, const QStringList &nameFilters, DirFilters filters, IteratorFlags flags)
    : DEnumerator(uri, nameFilters, filters, flags), d(new DLocalEnumeratorPrivate(this))
{
    registerFileInfoList(std::bind(&DLocalEnumerator::fileInfoList, this));
    registerHasNext(std::bind(&DLocalEnumerator::hasNext, this));
    registerNext(std::bind(&DLocalEnumerator::next, this));
    registerFileInfo(std::bind(&DLocalEnumerator::fileInfo, this));
    registerLastError(std::bind(&DLocalEnumerator::lastError, this));

    d->init();

    d->nameFilters = nameFilters;
    d->dirFilters = filters;
    d->iteratorFlags = flags;

    d->enumSubDir = d->iteratorFlags & DEnumerator::IteratorFlag::Subdirectories;
    d->enumLinks = d->iteratorFlags & DEnumerator::IteratorFlag::FollowSymlinks;
}

DLocalEnumerator::~DLocalEnumerator()
{
}

bool DLocalEnumerator::hasNext() const
{
    return d->hasNext();
}

QString DLocalEnumerator::next() const
{
    return d->next();
}

QSharedPointer<DFileInfo> DLocalEnumerator::fileInfo() const
{
    return d->fileInfo();
}

DFMIOError DLocalEnumerator::lastError() const
{
    return d->lastError();
}

QList<QSharedPointer<DFileInfo>> DLocalEnumerator::fileInfoList()
{
    return d->fileInfoList();
}
