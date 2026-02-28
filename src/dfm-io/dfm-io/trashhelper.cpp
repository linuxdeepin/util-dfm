// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <dfm-io/trashhelper.h>

#include <gio/gio.h>

#include <QDebug>
#include <QUrl>

BEGIN_IO_NAMESPACE
TrashHelper::TrashHelper()
{
}

TrashHelper::~TrashHelper()
{
}

void TrashHelper::setDeleteInfos(const QMap<QUrl, QSharedPointer<DeleteTimeInfo>> &infos)
{
    deleteInfos = infos;
}

bool TrashHelper::getTrashUrls(QList<QUrl> *trashUrls, QString *errorMsg)
{
    if (!trashUrls) {
        if (errorMsg)
            *errorMsg = "trash Urls list is nullptr!";
        qWarning() << "trash Urls list is nullptr!";
        return false;
    }

    GFileEnumerator *enumerator { nullptr };
    GFile *trash { nullptr };
    GError *error { nullptr };

    trash = g_file_new_for_uri("trash:///");
    if (!trash) {
        // pause and emit error msg
        if (errorMsg)
            *errorMsg = "fialed to create trash file!";
        qWarning() << "fialed to create trash file!";
        return false;
    }

    enumerator = g_file_enumerate_children(trash,
                                           G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_TRASH_DELETION_DATE "," G_FILE_ATTRIBUTE_TRASH_ORIG_PATH,
                                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                           nullptr, &error);
    // 错误处理
    if (!enumerator) {
        // pause and emit error msg
        qWarning() << "fialed to create trash iterator!";
        if (errorMsg)
            *errorMsg = error ? error->message : "fialed to create trash iterator!";
        if (error)
            g_error_free(error);
        g_object_unref(trash);
        return false;
    }

    GFileInfo *info { nullptr };
    GFile *item { nullptr };
    glong trash_time;
    const char *origpath;
    GFile *origfile { nullptr };
    trashUrls->clear();
    while ((info = g_file_enumerator_next_file(enumerator, nullptr, &error)) != nullptr) {
        /* Retrieve the original file uri */
        origpath = g_file_info_get_attribute_byte_string(info, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);
        origfile = g_file_new_for_path(origpath);

        if (!origfile) {
            g_object_unref(info);
            continue;
        }

        gchar *origfile_uri = g_file_get_uri(origfile);
        auto deleteinfo = deleteInfos.value(QUrl::fromPercentEncoding(origfile_uri));
        g_free(origfile_uri);
        
        if (!deleteinfo) {
            g_object_unref(origfile);
            g_object_unref(info);
            continue;
        }

        GDateTime *date;

        trash_time = 0;
        date = g_file_info_get_deletion_date(info);
        if (date) {
            trash_time = g_date_time_to_unix(date);
            g_date_time_unref(date);
        }

        if (deleteinfo->startTime <= trash_time && trash_time <= deleteinfo->endTime) {
            /* File in the trash */
            item = g_file_get_child(trash, g_file_info_get_name(info));
            if (!item) {
                g_object_unref(origfile);
                g_object_unref(info);
                continue;
            }

            gchar *item_uri = g_file_get_uri(item);
            trashUrls->append(QUrl(item_uri));
            g_free(item_uri);
            
            g_object_unref(item);
        }

        g_object_unref(origfile);
        g_object_unref(info);
        
        if (trashUrls->size() >= deleteInfos.size())
            break;
    }

    g_file_enumerator_close(enumerator, nullptr, nullptr);
    g_object_unref(enumerator);

    g_object_unref(trash);
    if (error) {
        if (errorMsg)
            *errorMsg = error->message;
        return false;
    }
    return true;
}

END_IO_NAMESPACE
