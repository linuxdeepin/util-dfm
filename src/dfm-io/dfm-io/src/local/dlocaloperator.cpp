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

#include "dfmio_global.h"
#include "local/dlocaloperator.h"
#include "local/dlocaloperator_p.h"
#include "local/dlocalhelper.h"

#include "core/doperator_p.h"

#include <gio/gio.h>
#include <glib/gstdio.h>

#include <QDebug>

USING_IO_NAMESPACE

DLocalOperatorPrivate::DLocalOperatorPrivate(DLocalOperator *q)
    : q(q)
{
}

DLocalOperatorPrivate::~DLocalOperatorPrivate()
{
}

bool DLocalOperatorPrivate::renameFile(const QString &new_name)
{
    const QUrl &url = q->uri();

    GError *gerror = nullptr;

    // name must deep copy, otherwise name freed and crash
    gchar *name = g_strdup(new_name.toLocal8Bit().data());

    GFile *gfile = makeGFile(url);

    GFile *gfile_ret = g_file_set_display_name(gfile, name, nullptr, &gerror);

    g_object_unref(gfile);
    g_free(name);

    if (!gfile_ret) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
        return false;
    }

    if (gerror)
        g_error_free(gerror);
    g_object_unref(gfile_ret);

    return true;
}

bool DLocalOperatorPrivate::renameFile(const QUrl &toUrl)
{
    const QUrl &fromUrl = q->uri();

    const std::string &fromStr = fromUrl.toLocalFile().toStdString();
    const std::string &toStr = toUrl.toLocalFile().toStdString();

    const bool ret = g_rename(fromStr.c_str(), toStr.c_str()) == 0;

    // set error info
    if (!ret)
        error.setCode(DFMIOErrorCode(DFM_IO_ERROR_PERMISSION_DENIED));

    return ret;
}

bool DLocalOperatorPrivate::copyFile(const QUrl &urlTo, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData)
{
    GError *gerror = nullptr;

    const QUrl &urlFrom = q->uri();

    GFile *gfile_from = makeGFile(urlFrom);
    GFile *gfile_to = makeGFile(urlTo);

    GFile *gfileTarget = nullptr;
    if (DLocalHelper::checkGFileType(gfile_to, G_FILE_TYPE_DIRECTORY)) {
        char *basename = g_file_get_basename(gfile_from);
        gfileTarget = g_file_get_child(gfile_to, basename);
        g_free(basename);
    } else {
        gfileTarget = makeGFile(urlTo);
    }
    g_object_unref(gfile_to);

    bool ret = g_file_copy(gfile_from, gfileTarget, GFileCopyFlags(flag), nullptr, func, progressCallbackData, &gerror);

    if (gerror) {
        setErrorFromGError(gerror);
        g_error_free(gerror);
    }

    g_object_unref(gfile_from);
    g_object_unref(gfileTarget);

    return ret;
}

bool DLocalOperatorPrivate::moveFile(const QUrl &to, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData)
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &from = q->uri();
    g_autoptr(GFile) gfile_from = makeGFile(from);

    g_autoptr(GFile) gfile_to = makeGFile(to);

    bool ret = g_file_move(gfile_from, gfile_to, GFileCopyFlags(flag), nullptr, func, progressCallbackData, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    return ret;
}

typedef struct
{
    DOperator::FileOperateCallbackFunc callback;
    gpointer user_data;
} OperateFileOp;

void RenameCallback(GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(user_data);
    GFile *gfile = (GFile *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    GFile *gfileRet = g_file_set_display_name_finish(gfile, res, &gerror);
    g_object_unref(gfileRet);
    if (data->callback)
        data->callback(!gerror, user_data);
}

void DLocalOperatorPrivate::renameFileAsync(const QString &newName, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &url = q->uri();

    // name must deep copy, otherwise name freed and crash
    g_autofree gchar *gname = g_strdup(newName.toLocal8Bit().data());

    g_autoptr(GFile) gfile = makeGFile(url);

    OperateFileOp *data = g_new0(OperateFileOp, 1);
    data->callback = operatefunc;
    data->user_data = userData;

    g_file_set_display_name_async(gfile, gname, ioPriority, nullptr, RenameCallback, data);
}

void CopyCallback(GObject *source_object,
                  GAsyncResult *res,
                  gpointer user_data)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(user_data);
    GFile *gfile = (GFile *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    gboolean succ = g_file_copy_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, user_data);
}

void DLocalOperatorPrivate::copyFileAsync(const QUrl &urlTo, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData,
                                          int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &urlFrom = q->uri();

    g_autoptr(GFile) gfile_from = makeGFile(urlFrom);
    g_autoptr(GFile) gfile_to = makeGFile(urlTo);

    g_autoptr(GFile) gfileTarget = nullptr;
    if (DLocalHelper::checkGFileType(gfile_to, G_FILE_TYPE_DIRECTORY)) {
        g_autofree char *basename = g_file_get_basename(gfile_from);
        gfileTarget = g_file_get_child(gfile_to, basename);
    } else {
        gfileTarget = makeGFile(urlTo);
    }

    OperateFileOp *data = g_new0(OperateFileOp, 1);
    data->callback = operatefunc;
    data->user_data = userData;

    g_file_copy_async(gfile_from, gfileTarget, GFileCopyFlags(flag), ioPriority, nullptr, func, progressCallbackData, CopyCallback, data);
}

void DLocalOperatorPrivate::moveFileAsync(const QUrl &to, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc progressFunc, void *progressData,
                                          int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    Q_UNUSED(ioPriority)
    Q_UNUSED(operatefunc)
    Q_UNUSED(userData)

    // TODO(lanxs) gio no move async func
    moveFile(to, flag, progressFunc, progressData);
}

bool DLocalOperatorPrivate::trashFile()
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    bool ret = g_file_trash(gfile, nullptr, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    return ret;
}

bool DLocalOperatorPrivate::deleteFile()
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    bool ret = g_file_delete(gfile, nullptr, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    return ret;
}

bool DLocalOperatorPrivate::restoreFile(DOperator::ProgressCallbackFunc func, void *progressCallbackData)
{
    GError *gerror = nullptr;

    const QUrl &uri = q->uri();
    GFile *file = makeGFile(uri);

    GFileInfo *gfileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH, G_FILE_QUERY_INFO_NONE, nullptr, &gerror);
    g_object_unref(file);

    if (!gfileinfo) {
        if (gerror) {
            setErrorFromGError(gerror);
            g_error_free(gerror);
        }
        return false;
    }

    const std::string &src_path = g_file_info_get_attribute_byte_string(gfileinfo, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);

    if (src_path.empty()) {
        g_object_unref(gfileinfo);
        return false;
    }

    QUrl url_dest;
    url_dest.setPath(QString::fromStdString(src_path.c_str()));
    url_dest.setScheme(QString("file"));

    bool ret = moveFile(url_dest, DFile::CopyFlag::kNone, func, progressCallbackData);

    g_object_unref(gfileinfo);

    return ret;
}

void TrashCallback(GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(user_data);
    GFile *gfile = (GFile *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    bool succ = g_file_trash_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, user_data);
}

void DLocalOperatorPrivate::trashFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    OperateFileOp *data = g_new0(OperateFileOp, 1);
    data->callback = operatefunc;
    data->user_data = userData;

    g_file_trash_async(gfile, ioPriority, nullptr, TrashCallback, data);
}

void DeleteCallback(GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(user_data);
    GFile *gfile = (GFile *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    bool succ = g_file_delete_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, user_data);
}

void DLocalOperatorPrivate::deleteFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    OperateFileOp *data = g_new0(OperateFileOp, 1);
    data->callback = operatefunc;
    data->user_data = userData;

    g_file_delete_async(gfile, ioPriority, nullptr, DeleteCallback, data);
}

void DLocalOperatorPrivate::restoreFileAsync(DOperator::ProgressCallbackFunc progressFunc, void *progressData,
                                             int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    // TODO(lanxs)
    Q_UNUSED(ioPriority)
    Q_UNUSED(operatefunc)
    Q_UNUSED(userData)
    restoreFile(progressFunc, progressData);
}

bool DLocalOperatorPrivate::touchFile()
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    // if file exist, return failed
    g_autoptr(GFileOutputStream) stream = g_file_create(gfile, GFileCreateFlags::G_FILE_CREATE_REPLACE_DESTINATION, nullptr, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    return stream != nullptr;
}

bool DLocalOperatorPrivate::makeDirectory()
{
    // only create direct path
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    bool ret = g_file_make_directory(gfile, nullptr, &gerror);

    if (gerror)
        setErrorFromGError(gerror);

    return ret;
}

bool DLocalOperatorPrivate::createLink(const QUrl &link)
{
    g_autoptr(GError) gerror = nullptr;

    g_autoptr(GFile) gfile = makeGFile(link);

    const QUrl &uri = q->uri();
    const QString &linkValue = uri.toLocalFile();

    bool ret = g_file_make_symbolic_link(gfile, linkValue.toLocal8Bit().data(), nullptr, &gerror);

    if (!ret) {
        setErrorFromGError(gerror);
    }

    return ret;
}

void TouchCallback(GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(user_data);
    GFile *gfile = (GFile *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GFileOutputStream) stream = g_file_create_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(!stream, user_data);
}

void DLocalOperatorPrivate::touchFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    OperateFileOp *data = g_new0(OperateFileOp, 1);
    data->callback = operatefunc;
    data->user_data = userData;

    g_file_create_async(gfile, GFileCreateFlags::G_FILE_CREATE_REPLACE_DESTINATION, ioPriority, nullptr, TouchCallback, data);
}

void MakeDirCallback(GObject *source_object,
                     GAsyncResult *res,
                     gpointer user_data)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(user_data);
    GFile *gfile = (GFile *)(source_object);
    g_autoptr(GError) gerror = nullptr;
    bool succ = g_file_make_directory_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, user_data);
}

void DLocalOperatorPrivate::makeDirectoryAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    // only create direct path
    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    OperateFileOp *data = g_new0(OperateFileOp, 1);
    data->callback = operatefunc;
    data->user_data = userData;

    g_file_make_directory_async(gfile, ioPriority, nullptr, MakeDirCallback, data);
}

void DLocalOperatorPrivate::createLinkAsync(const QUrl &link, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    // TODO(lanxs)
    Q_UNUSED(ioPriority)
    Q_UNUSED(operatefunc)
    Q_UNUSED(userData)
    createLink(link);
}

bool DLocalOperatorPrivate::setFileInfo(const DFileInfo &fileInfo)
{
    const QUrl &uri = q->uri();
    g_autoptr(GFile) gfile = makeGFile(uri);

    bool ret = true;
    for (const auto &[key, value] : DFileInfo::attributeInfoMap) {
        g_autoptr(GError) gerror = nullptr;
        bool succ = DLocalHelper::setAttributeByGFile(gfile, key, fileInfo.attribute(key, nullptr), &gerror);
        if (!succ)
            ret = false;
        if (gerror)
            setErrorFromGError(gerror);
    }

    return ret;
}

bool DLocalOperatorPrivate::cancel()
{
    return true;
}

DFMIOError DLocalOperatorPrivate::lastError()
{
    return error;
}

GFile *DLocalOperatorPrivate::makeGFile(const QUrl &url)
{
    return g_file_new_for_uri(url.toString().toLocal8Bit().data());
}

void DLocalOperatorPrivate::freeCancellable(GCancellable *gcancellable)
{
}

void DLocalOperatorPrivate::setErrorFromGError(GError *gerror)
{
    error.setCode(DFMIOErrorCode(gerror->code));

    //qWarning() << QString::fromLocal8Bit(gerror->message);
}

DLocalOperator::DLocalOperator(const QUrl &uri)
    : DOperator(uri), d(new DLocalOperatorPrivate(this))
{
    registerRenameFile(bind_field(this, static_cast<bool (DLocalOperator::*)(const QString &)>(&DLocalOperator::renameFile)));
    registerRenameFileByStd(bind_field(this, static_cast<bool (DLocalOperator::*)(const QUrl &)>(&DLocalOperator::renameFile)));
    registerCopyFile(bind_field(this, &DLocalOperator::copyFile));
    registerMoveFile(bind_field(this, &DLocalOperator::moveFile));
    registerRenameFileAsync(bind_field(this, &DLocalOperator::renameFileAsync));
    registerCopyFileAsync(bind_field(this, &DLocalOperator::copyFileAsync));
    registerCopyFileAsync(bind_field(this, &DLocalOperator::copyFileAsync));
    registerMoveFileAsync(bind_field(this, &DLocalOperator::moveFileAsync));

    registerTrashFile(bind_field(this, &DLocalOperator::trashFile));
    registerDeleteFile(bind_field(this, &DLocalOperator::deleteFile));
    registerRestoreFile(bind_field(this, &DLocalOperator::restoreFile));
    registerTrashFileAsync(bind_field(this, &DLocalOperator::trashFileAsync));
    registerDeleteFileAsync(bind_field(this, &DLocalOperator::deleteFileAsync));
    registerRestoreFileAsync(bind_field(this, &DLocalOperator::restoreFileAsync));

    registerTouchFile(bind_field(this, &DLocalOperator::touchFile));
    registerMakeDirectory(bind_field(this, &DLocalOperator::makeDirectory));
    registerCreateLink(bind_field(this, &DLocalOperator::createLink));
    registerTouchFileAsync(bind_field(this, &DLocalOperator::touchFileAsync));
    registerMakeDirectoryAsync(bind_field(this, &DLocalOperator::makeDirectoryAsync));
    registerCreateLinkAsync(bind_field(this, &DLocalOperator::createLinkAsync));

    registerSetFileInfo(bind_field(this, &DLocalOperator::setFileInfo));

    registerCancel(bind_field(this, &DLocalOperator::cancel));
    registerLastError(bind_field(this, &DLocalOperator::lastError));
}

DLocalOperator::~DLocalOperator()
{
}

bool DLocalOperator::renameFile(const QString &newName)
{
    return d->renameFile(newName);
}

bool DLocalOperator::renameFile(const QUrl &toUrl)
{
    return d->renameFile(toUrl);
}

bool DLocalOperator::copyFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func, void *progressCallbackData)
{
    return d->copyFile(destUri, flag, func, progressCallbackData);
}

bool DLocalOperator::moveFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func, void *progressCallbackData)
{
    return d->moveFile(destUri, flag, func, progressCallbackData);
}

void DLocalOperator::renameFileAsync(const QString &newName, int ioPriority, FileOperateCallbackFunc func, void *userData)
{
    d->renameFileAsync(newName, ioPriority, func, userData);
}

void DLocalOperator::copyFileAsync(const QUrl &destUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData,
                                   int ioPriority, FileOperateCallbackFunc operatefunc, void *userData)
{
    d->copyFileAsync(destUri, flag, func, progressCallbackData, ioPriority, operatefunc, userData);
}

void DLocalOperator::moveFileAsync(const QUrl &destUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData,
                                   int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    d->moveFileAsync(destUri, flag, func, progressCallbackData, ioPriority, operatefunc, userData);
}

bool DLocalOperator::trashFile()
{
    return d->trashFile();
}

bool DLocalOperator::deleteFile()
{
    return d->deleteFile();
}

bool DLocalOperator::restoreFile(ProgressCallbackFunc func, void *progressCallbackData)
{
    return d->restoreFile(func, progressCallbackData);
}

void DLocalOperator::trashFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    d->trashFileAsync(ioPriority, operatefunc, userData);
}

void DLocalOperator::deleteFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    return d->deleteFileAsync(ioPriority, operatefunc, userData);
}

void DLocalOperator::restoreFileAsync(DOperator::ProgressCallbackFunc func, void *progressCallbackData,
                                      int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    d->restoreFileAsync(func, progressCallbackData, ioPriority, operatefunc, userData);
}

bool DLocalOperator::touchFile()
{
    return d->touchFile();
}

bool DLocalOperator::makeDirectory()
{
    return d->makeDirectory();
}

bool DLocalOperator::createLink(const QUrl &link)
{
    return d->createLink(link);
}

void DLocalOperator::touchFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    d->touchFileAsync(ioPriority, operatefunc, userData);
}

void DLocalOperator::makeDirectoryAsync(int ioPriority, DOperator::FileOperateCallbackFunc operateFunc, void *userData)
{
    d->makeDirectoryAsync(ioPriority, operateFunc, userData);
}

void DLocalOperator::createLinkAsync(const QUrl &link, int ioPriority, DOperator::FileOperateCallbackFunc operateFunc, void *userData)
{
    d->createLinkAsync(link, ioPriority, operateFunc, userData);
}

bool DLocalOperator::setFileInfo(const DFileInfo &fileInfo)
{
    return d->setFileInfo(fileInfo);
}

bool DLocalOperator::cancel()
{
    return d->cancel();
}

DFMIOError DLocalOperator::lastError() const
{
    return d->lastError();
}
