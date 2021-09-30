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
    g_object_unref(enumerator);
}

QList<QSharedPointer<DFileInfo>> DLocalEnumeratorPrivate::fileInfoList()
{
    GFileEnumerator *enumerator = nullptr;
    GError *gerror = nullptr;

    GFile *gfile = g_file_new_for_uri(q->uri().toString().toStdString().c_str());

    enumerator = g_file_enumerate_children(gfile,
                                           "*",
                                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                           nullptr,
                                           &gerror);

    if (nullptr == enumerator) {
        if (gerror) {
            qWarning() << gerror->message;
            dfmError.setCode(DFMIOErrorCode(gerror->code));

            g_error_free(gerror);
        }
        g_object_unref(gfile);
        return list_;
    }
    g_object_unref(gfile);

    GFile *gfileIn = nullptr;
    GFileInfo *gfileInfoIn = nullptr;

    while (g_file_enumerator_iterate(enumerator, &gfileInfoIn, &gfileIn, nullptr, &gerror)) {
        if (!gfileInfoIn)
            break;

        char *uri = g_file_get_uri(gfileIn);
        QSharedPointer<DFileInfo> info = DLocalHelper::getFileInfoByUri(uri);
        g_free(uri);

        if (info)
            list_.append(info);

        if (gerror) {
            qWarning() << "error:" << gerror->message;
            dfmError.setCode(DFMIOErrorCode(gerror->code));

            g_error_free(gerror);
            gerror = nullptr;
        }
    }

    if (gerror) {
        qWarning() << gerror->message;
        dfmError.setCode(DFMIOErrorCode(gerror->code));
        g_error_free(gerror);
    }
    g_object_unref(enumerator);

    return list_;
}

bool DLocalEnumeratorPrivate::hasNext()
{
    GError *gerror = nullptr;
    GFileInfo *gfileInfoIn = nullptr;

    bool hasNext = g_file_enumerator_iterate(enumerator, &gfileInfoIn, &fileNext, nullptr, &gerror);
    if (hasNext) {

        if (!gfileInfoIn)
            return false;

        char *uri = g_file_get_uri(fileNext);

        fileInfoNext = DLocalHelper::getFileInfoByUri(uri);
        g_free(uri);

        return true;
    }

    if (gerror) {
        qWarning() << gerror->message;
        dfmError.setCode(DFMIOErrorCode(gerror->code));
        g_error_free(gerror);
    }
    return false;
}

QString DLocalEnumeratorPrivate::next() const
{
    char *gpath = g_file_get_path(fileNext);
    QString qsPath = QString::fromLocal8Bit(gpath);
    g_free(gpath);
    return qsPath;
}

QSharedPointer<DFileInfo> DLocalEnumeratorPrivate::fileInfo() const
{
    return fileInfoNext;
}

void DLocalEnumeratorPrivate::init()
{
    GError *gerror = nullptr;

    const QString &uriPath = q->uri().toString();
    GFile *gfile = g_file_new_for_uri(uriPath.toLocal8Bit().data());

    enumerator = g_file_enumerate_children(gfile,
                                           "*",
                                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                           nullptr,
                                           &gerror);

    if (nullptr == enumerator) {
        if (gerror) {
            qWarning() << gerror->message;
            dfmError.setCode(DFMIOErrorCode(gerror->code));

            g_error_free(gerror);
        }
    }
    g_object_unref(gfile);
}

DLocalEnumerator::DLocalEnumerator(const QUrl &uri)
    : DEnumerator(uri)
    , d(new DLocalEnumeratorPrivate(this))
{
    registerFileInfoList(std::bind(&DLocalEnumerator::fileInfoList, this));
    registerHasNext(std::bind(&DLocalEnumerator::hasNext, this));
    registerNext(std::bind(&DLocalEnumerator::next, this));
    registerFileInfo(std::bind(&DLocalEnumerator::fileInfo, this));

    d->init();
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

QList<QSharedPointer<DFileInfo> > DLocalEnumerator::fileInfoList()
{
    return d->fileInfoList();
}
