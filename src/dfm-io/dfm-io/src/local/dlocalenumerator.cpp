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

DLocalEnumeratorPrivate::DLocalEnumeratorPrivate(DLocalEnumerator *ptr)
    : q_ptr(ptr)
{
}

DLocalEnumeratorPrivate::~DLocalEnumeratorPrivate()
{
}

QList<QSharedPointer<DFileInfo>> DLocalEnumeratorPrivate::fileInfoList()
{
    if (done)
        return list_;

    GFileEnumerator *enumerator = nullptr;
    GFileInfo *gfileinfo = nullptr;
    GError *error = nullptr;

    GFile *gfile = g_file_new_for_uri(q_ptr->uri().toString().toStdString().c_str());
    enumerator = g_file_enumerate_children(gfile,
                                           "*",
                                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                           nullptr,
                                           &error);

    if (nullptr == enumerator) {
        if (error) {
            qWarning() << error->message;
            dfmError.setCode(DFMIOErrorCode(error->code));

            g_error_free(error);
        }
        g_object_unref(gfile);
        return list_;
    }
    if (error)
        g_error_free(error);
    g_object_unref(gfile);

    error = nullptr;
    while ((gfileinfo = g_file_enumerator_next_file(enumerator, nullptr, &error)) != nullptr) {
        auto info = DLocalHelper::getFileInfoFromGFileInfo(gfileinfo);
        g_object_unref(gfileinfo);

        if (info)
            list_.append(QSharedPointer<DFileInfo>(info));

        if (error) {
            qWarning() << "error:" << error->message;
            dfmError.setCode(DFMIOErrorCode(error->code));
            g_error_free(error);
        }
        error = nullptr;
    }
    g_file_enumerator_close(enumerator, nullptr, &error);
    if (error) {
        qWarning() << error->message;
        dfmError.setCode(DFMIOErrorCode(error->code));
        g_error_free(error);
    }
    g_object_unref(enumerator);

    done = true;

    return list_;
}

DLocalEnumerator::DLocalEnumerator(const QUrl &uri)
    : DEnumerator(uri)
    , d_ptr(new DLocalEnumeratorPrivate(this))
{
    registerFileInfoList(std::bind(&DLocalEnumerator::fileInfoList, this));
}

DLocalEnumerator::~DLocalEnumerator()
{
}

QList<QSharedPointer<DFileInfo> > DLocalEnumerator::fileInfoList()
{
    Q_D(DLocalEnumerator);
    return d->fileInfoList();
}
