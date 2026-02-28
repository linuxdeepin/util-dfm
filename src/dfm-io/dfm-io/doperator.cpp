// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/doperator_p.h"

#include "utils/dlocalhelper.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include <glib/gstdio.h>
#include <errno.h>

USING_IO_NAMESPACE

/************************************************
 * DOperatorPrivate
 ***********************************************/

DOperatorPrivate::DOperatorPrivate(DOperator *q)
    : q(q)
{
}

DOperatorPrivate::~DOperatorPrivate()
{
}

void DOperatorPrivate::setErrorFromGError(GError *gerror)
{
    if (!gerror)
        return;
    error.setCode(DFMIOErrorCode(gerror->code));
    if (error.code() == DFMIOErrorCode::DFM_IO_ERROR_FAILED) {
        QString strErr(gerror->message);
        if (strErr.contains(':'))
            strErr = strErr.left(strErr.indexOf(":")) + strErr.mid(strErr.lastIndexOf(":"));
        error.setMessage(strErr);
    }
}

void DOperatorPrivate::setErrorFromErrno(int errnoValue)
{
    DFMIOErrorCode errorCode;
    switch (errnoValue) {
    case EACCES:
    case EPERM:
        errorCode = DFM_IO_ERROR_PERMISSION_DENIED;
        break;
    case ENOENT:
        errorCode = DFM_IO_ERROR_NOT_FOUND;
        break;
    case EEXIST:
    case ENOTEMPTY:
        errorCode = DFM_IO_ERROR_EXISTS;
        break;
    case EISDIR:
        errorCode = DFM_IO_ERROR_IS_DIRECTORY;
        break;
    case ENOTDIR:
        errorCode = DFM_IO_ERROR_NOT_DIRECTORY;
        break;
    case EROFS:
        errorCode = DFM_IO_ERROR_READ_ONLY;
        break;
    case ENOSPC:
        errorCode = DFM_IO_ERROR_NO_SPACE;
        break;
    case ENAMETOOLONG:
        errorCode = DFM_IO_ERROR_FILENAME_TOO_LONG;
        break;
    case EINVAL:
        errorCode = DFM_IO_ERROR_INVALID_ARGUMENT;
        break;
    case EBUSY:
        errorCode = DFM_IO_ERROR_BUSY;
        break;
    case EXDEV:
        // Cross-device rename not supported by g_rename
        errorCode = DFM_IO_ERROR_NOT_SUPPORTED;
        break;
    default:
        errorCode = DFM_IO_ERROR_FAILED;
        break;
    }

    error.setCode(errorCode);
}

GFile *DOperatorPrivate::makeGFile(const QUrl &url)
{
    return DLocalHelper::createGFile(url);
}

void DOperatorPrivate::checkAndResetCancel()
{
    if (gcancellable) {
        if (!g_cancellable_is_cancelled(gcancellable))
            g_cancellable_cancel(gcancellable);
        g_cancellable_reset(gcancellable);
        return;
    }
    gcancellable = g_cancellable_new();
}

void DOperatorPrivate::renameCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(userData);
    GFile *gfile = G_FILE(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    auto dataUser = data->userData;
    GFile *gfileRet = g_file_set_display_name_finish(gfile, res, &gerror);
    g_object_unref(gfileRet);
    if (data->callback)
        data->callback(!gerror, dataUser);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

void DOperatorPrivate::copyCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(userData);
    GFile *gfile = G_FILE(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    auto dataUser = data->userData;
    gboolean succ = g_file_copy_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, dataUser);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

void DOperatorPrivate::trashCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(userData);
    GFile *gfile = G_FILE(sourceObject);
    auto dataUser = data->userData;
    g_autoptr(GError) gerror = nullptr;
    bool succ = g_file_trash_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, dataUser);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

void DOperatorPrivate::deleteCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(userData);
    GFile *gfile = G_FILE(sourceObject);
    auto dataUser = data->userData;
    g_autoptr(GError) gerror = nullptr;
    bool succ = g_file_delete_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, dataUser);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

void DOperatorPrivate::touchCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(userData);
    auto dataUser = data->userData;
    GFile *gfile = G_FILE(sourceObject);
    g_autoptr(GError) gerror = nullptr;
    g_autoptr(GFileOutputStream) stream = g_file_create_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(!stream, dataUser);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

void DOperatorPrivate::makeDirCallback(GObject *sourceObject, GAsyncResult *res, gpointer userData)
{
    OperateFileOp *data = static_cast<OperateFileOp *>(userData);
    GFile *gfile = G_FILE(sourceObject);
    auto dataUser = data->userData;
    g_autoptr(GError) gerror = nullptr;
    bool succ = g_file_make_directory_finish(gfile, res, &gerror);
    if (data->callback)
        data->callback(succ, dataUser);

    data->callback = nullptr;
    data->userData = nullptr;
    g_free(data);
}

/************************************************
 * DOperator
 ***********************************************/

DOperator::DOperator(const QUrl &uri)
    : d(new DOperatorPrivate(this))
{
    d->uri = uri;
}

DOperator::~DOperator()
{
    if (d->gcancellable) {
        if (!g_cancellable_is_cancelled(d->gcancellable))
            g_cancellable_cancel(d->gcancellable);
        g_object_unref(d->gcancellable);
        d->gcancellable = nullptr;
    }
}

QUrl DOperator::uri() const
{
    return d->uri;
}

bool DOperator::renameFile(const QString &newName)
{
    const QUrl &url = uri();

    GError *gerror = nullptr;

    // name must deep copy, otherwise name freed and crash
    gchar *name = g_strdup(newName.toLocal8Bit().data());

    GFile *gfile = d->makeGFile(url);

    GFile *gfile_ret = g_file_set_display_name(gfile, name, nullptr, &gerror);

    g_object_unref(gfile);
    g_free(name);

    if (!gfile_ret) {
        d->setErrorFromGError(gerror);
        g_error_free(gerror);
        return false;
    }

    if (gerror)
        g_error_free(gerror);
    g_object_unref(gfile_ret);

    return true;
}

bool DOperator::renameFile(const QUrl &toUrl)
{
    const QUrl &fromUrl = uri();

    const std::string &fromStr = fromUrl.toLocalFile().toStdString();
    const std::string &toStr = toUrl.toLocalFile().toStdString();

    if (fromStr.empty() || toStr.empty()) {
        d->error.setCode(DFM_IO_ERROR_INVALID_FILENAME);
        return false;
    }

    const bool ret = g_rename(fromStr.c_str(), toStr.c_str()) == 0;

    // set error info based on errno
    if (!ret)
        d->setErrorFromErrno(errno);

    return ret;
}

bool DOperator::copyFile(const QUrl &destUri, dfmio::DFile::CopyFlags flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData)
{
    GError *gerror = nullptr;

    const QUrl &urlFrom = uri();

    GFile *gfile_from = d->makeGFile(urlFrom);
    GFile *gfile_to = d->makeGFile(destUri);

    GFile *gfileTarget = nullptr;
    if (DLocalHelper::checkGFileType(gfile_to, G_FILE_TYPE_DIRECTORY)) {
        char *basename = g_file_get_basename(gfile_from);
        gfileTarget = g_file_get_child(gfile_to, basename);
        g_free(basename);
    } else {
        gfileTarget = d->makeGFile(destUri);
    }
    g_object_unref(gfile_to);

    d->checkAndResetCancel();
    bool ret = g_file_copy(gfile_from, gfileTarget, GFileCopyFlags(static_cast<uint8_t>(flag)), d->gcancellable, func, progressCallbackData, &gerror);

    if (gerror) {
        d->setErrorFromGError(gerror);
        g_error_free(gerror);
    }

    g_object_unref(gfile_from);
    g_object_unref(gfileTarget);

    return ret;
}

bool DOperator::moveFile(const QUrl &destUri, dfmio::DFile::CopyFlags flag, DOperator::ProgressCallbackFunc func, void *progressCallbackData)
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &from = uri();
    g_autoptr(GFile) gfile_from = d->makeGFile(from);

    g_autoptr(GFile) gfile_to = d->makeGFile(destUri);

    bool ret = g_file_move(gfile_from, gfile_to, GFileCopyFlags(static_cast<uint8_t>(flag)), nullptr, func, progressCallbackData, &gerror);

    if (gerror)
        d->setErrorFromGError(gerror);

    return ret;
}

void DOperator::renameFileAsync(const QString &newName, int ioPriority, DOperator::FileOperateCallbackFunc func, void *userData)
{
    const QUrl &url = uri();

    // name must deep copy, otherwise name freed and crash
    g_autofree gchar *gname = g_strdup(newName.toLocal8Bit().data());

    g_autoptr(GFile) gfile = d->makeGFile(url);

    DOperatorPrivate::OperateFileOp *data = g_new0(DOperatorPrivate::OperateFileOp, 1);
    data->callback = func;
    data->userData = userData;

    g_file_set_display_name_async(gfile, gname, ioPriority,
                                  nullptr, DOperatorPrivate::renameCallback, data);
}

void DOperator::copyFileAsync(const QUrl &destUri, DFile::CopyFlags flag, DOperator::ProgressCallbackFunc progressfunc, void *progressCallbackData, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &urlFrom = uri();

    g_autoptr(GFile) gfile_from = d->makeGFile(urlFrom);
    g_autoptr(GFile) gfile_to = d->makeGFile(destUri);

    g_autoptr(GFile) gfileTarget = nullptr;
    if (DLocalHelper::checkGFileType(gfile_to, G_FILE_TYPE_DIRECTORY)) {
        g_autofree char *basename = g_file_get_basename(gfile_from);
        gfileTarget = g_file_get_child(gfile_to, basename);
    } else {
        gfileTarget = d->makeGFile(destUri);
    }

    DOperatorPrivate::OperateFileOp *data = g_new0(DOperatorPrivate::OperateFileOp, 1);
    data->callback = operatefunc;
    data->userData = userData;

    g_file_copy_async(gfile_from, gfileTarget, GFileCopyFlags(static_cast<uint8_t>(flag)), ioPriority,
                      nullptr, progressfunc, progressCallbackData, DOperatorPrivate::copyCallback, data);
}

void DOperator::moveFileAsync(const QUrl &destUri, dfmio::DFile::CopyFlags flag, DOperator::ProgressCallbackFunc progressFunc, void *progressCallbackData, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    // TODO:
    // since 2.72, but current gio version is 2.58
    // g_file_move_async(gfile_from, gfile_to, GFileCopyFlags(flag), ioPriority, nullptr, progressFunc, progressData, nullptr, userData);

    Q_UNUSED(ioPriority)
    Q_UNUSED(operatefunc)
    Q_UNUSED(userData)
    bool ret = moveFile(destUri, flag, progressFunc, progressCallbackData);
    if (operatefunc)
        operatefunc(ret, userData);
}

QString DOperator::trashFile()
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    QString targetTrashTime = QString::number(QDateTime::currentSecsSinceEpoch()) + "-";
    bool ret = g_file_trash(gfile, nullptr, &gerror);
    targetTrashTime.append(QString::number(QDateTime::currentSecsSinceEpoch()));
    if (ret)
        return targetTrashTime;

    if (gerror)
        d->setErrorFromGError(gerror);

    return QString();
}

bool DOperator::deleteFile()
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    bool ret = g_file_delete(gfile, nullptr, &gerror);

    if (gerror)
        d->setErrorFromGError(gerror);

    return ret;
}

bool DOperator::restoreFile(DOperator::ProgressCallbackFunc func, void *progressCallbackData)
{
    GError *gerror = nullptr;

    const QUrl &uri = this->uri();
    GFile *file = d->makeGFile(uri);

    GFileInfo *gfileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH, G_FILE_QUERY_INFO_NONE, nullptr, &gerror);
    g_object_unref(file);

    if (!gfileinfo) {
        if (gerror) {
            d->setErrorFromGError(gerror);
            g_error_free(gerror);
        }
        return false;
    }

    const char *srcPath = g_file_info_get_attribute_byte_string(gfileinfo, G_FILE_ATTRIBUTE_TRASH_ORIG_PATH);

    if (srcPath == nullptr) {
        g_object_unref(gfileinfo);
        return false;
    }

    QUrl urlDest;
    urlDest.setPath(QString::fromLocal8Bit(srcPath));
    urlDest.setScheme(QString("file"));

    bool ret = moveFile(urlDest, DFile::CopyFlag::kNone, func, progressCallbackData);

    g_object_unref(gfileinfo);

    return ret;
}

void DOperator::trashFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    DOperatorPrivate::OperateFileOp *data = g_new0(DOperatorPrivate::OperateFileOp, 1);
    data->callback = operatefunc;
    data->userData = userData;

    g_file_trash_async(gfile, ioPriority, nullptr, DOperatorPrivate::trashCallback, data);
}

void DOperator::deleteFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    DOperatorPrivate::OperateFileOp *data = g_new0(DOperatorPrivate::OperateFileOp, 1);
    data->callback = operatefunc;
    data->userData = userData;

    g_file_delete_async(gfile, ioPriority, nullptr, DOperatorPrivate::deleteCallback, data);
}

void DOperator::restoreFileAsync(DOperator::ProgressCallbackFunc func, void *progressCallbackData, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    // TODO: impl me!
    Q_UNUSED(ioPriority)
    Q_UNUSED(operatefunc)
    Q_UNUSED(userData)
    restoreFile(func, progressCallbackData);
}

bool DOperator::touchFile()
{
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    // if file exist, return failed
    g_autoptr(GFileOutputStream) stream = g_file_create(gfile, GFileCreateFlags::G_FILE_CREATE_REPLACE_DESTINATION, nullptr, &gerror);

    if (gerror)
        d->setErrorFromGError(gerror);

    return stream != nullptr;
}

bool DOperator::makeDirectory()
{
    // only create direct path
    g_autoptr(GError) gerror = nullptr;

    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    bool ret = g_file_make_directory(gfile, nullptr, &gerror);

    if (gerror)
        d->setErrorFromGError(gerror);

    return ret;
}

bool DOperator::createLink(const QUrl &link)
{
    g_autoptr(GError) gerror = nullptr;

    g_autoptr(GFile) gfile = d->makeGFile(link);

    const QUrl &uri = this->uri();
    const QString &linkValue = uri.toLocalFile();

    bool ret = g_file_make_symbolic_link(gfile, linkValue.toLocal8Bit().data(), nullptr, &gerror);

    if (!ret)
        d->setErrorFromGError(gerror);

    return ret;
}

void DOperator::touchFileAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    DOperatorPrivate::OperateFileOp *data = g_new0(DOperatorPrivate::OperateFileOp, 1);
    data->callback = operatefunc;
    data->userData = userData;

    g_file_create_async(gfile, GFileCreateFlags::G_FILE_CREATE_REPLACE_DESTINATION, ioPriority, nullptr, DOperatorPrivate::touchCallback, data);
}

void DOperator::makeDirectoryAsync(int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    // only create direct path
    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    DOperatorPrivate::OperateFileOp *data = g_new0(DOperatorPrivate::OperateFileOp, 1);
    data->callback = operatefunc;
    data->userData = userData;

    g_file_make_directory_async(gfile, ioPriority, nullptr, DOperatorPrivate::makeDirCallback, data);
}

void DOperator::createLinkAsync(const QUrl &link, int ioPriority, DOperator::FileOperateCallbackFunc operatefunc, void *userData)
{
    // TODO: imple me!
    Q_UNUSED(ioPriority)
    Q_UNUSED(operatefunc)
    Q_UNUSED(userData)
    createLink(link);
}

bool DOperator::setFileInfo(const DFileInfo &fileInfo)
{
    const QUrl &uri = this->uri();
    g_autoptr(GFile) gfile = d->makeGFile(uri);

    bool ret = true;
    for (const auto &[key, value] : DLocalHelper::attributeInfoMapFunc()) {
        g_autoptr(GError) gerror = nullptr;
        bool succ = DLocalHelper::setAttributeByGFile(gfile, key, fileInfo.attribute(key, nullptr), &gerror);
        if (!succ)
            ret = false;
        if (gerror)
            d->setErrorFromGError(gerror);
    }

    return ret;
}

bool DOperator::cancel()
{
    if (d->gcancellable && !g_cancellable_is_cancelled(d->gcancellable))
        g_cancellable_cancel(d->gcancellable);

    return true;
}

DFMIOError DOperator::lastError() const
{
    return d->error;
}
