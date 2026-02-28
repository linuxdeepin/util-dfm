// SPDX-FileCopyrightText: 2020 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOPERATOR_H
#define DOPERATOR_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/dfile.h>
#include <dfm-io/error/error.h>

#include <QUrl>
#include <QSharedPointer>

#include <functional>

BEGIN_IO_NAMESPACE

class DOperatorPrivate;
class DFileInfo;

class DOperator
{
public:
    // callback, use function pointer
    using ProgressCallbackFunc = void (*)(int64_t, int64_t, void *);   // current_num_bytes, total_num_bytes, user_data
    using FileOperateCallbackFunc = void (*)(bool, void *);

public:
    explicit DOperator(const QUrl &uri);
    virtual ~DOperator();

    QUrl uri() const;

    bool renameFile(const QString &newName);
    bool renameFile(const QUrl &toUrl);
    bool copyFile(const QUrl &destUri, DFile::CopyFlags flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    bool moveFile(const QUrl &destUri, DFile::CopyFlags flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    // async
    void renameFileAsync(const QString &newName, int ioPriority = 0, FileOperateCallbackFunc func = nullptr, void *userData = nullptr);
    void copyFileAsync(const QUrl &destUri, DFile::CopyFlags flag, ProgressCallbackFunc progressfunc = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void moveFileAsync(const QUrl &destUri, DFile::CopyFlags flag, ProgressCallbackFunc progressFunc = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    QString trashFile();
    bool deleteFile();
    bool restoreFile(ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    // async
    void trashFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void deleteFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void restoreFileAsync(ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                          int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    bool touchFile();
    bool makeDirectory();
    bool createLink(const QUrl &link);
    // async
    void touchFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void makeDirectoryAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void createLinkAsync(const QUrl &link, int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    bool setFileInfo(const DFileInfo &fileInfo);

    bool cancel();
    DFMIOError lastError() const;

private:
    QScopedPointer<DOperatorPrivate> d;
};

END_IO_NAMESPACE

#endif   // DOPERATOR_H
