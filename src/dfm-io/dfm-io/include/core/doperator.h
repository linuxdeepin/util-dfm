// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOPERATOR_H
#define DOPERATOR_H

#include "dfmio_global.h"
#include "core/dfile.h"
#include "error/error.h"

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

    // register function, use std::function
    using RenameFileFunc = std::function<bool(const QString &)>;
    using RenameFileByStdFunc = std::function<bool(const QUrl &)>;
    using RenameFileFuncAsync = std::function<void(const QString &, int, FileOperateCallbackFunc, void *)>;
    using CopyFileFunc = std::function<bool(const QUrl &, DFile::CopyFlag, ProgressCallbackFunc, void *)>;
    using CopyFileFuncAsync = std::function<void(const QUrl &, DFile::CopyFlag, ProgressCallbackFunc, void *, int, FileOperateCallbackFunc, void *)>;
    using MoveFileFunc = std::function<bool(const QUrl &, DFile::CopyFlag, ProgressCallbackFunc, void *)>;
    using MoveFileFuncAsync = std::function<void(const QUrl &, DFile::CopyFlag, ProgressCallbackFunc, void *, int, FileOperateCallbackFunc, void *)>;

    using TrashFileFunc = std::function<QString()>;
    using TrashFileFuncAsync = std::function<void(int, FileOperateCallbackFunc, void *)>;
    using DeleteFileFunc = std::function<bool()>;
    using DeleteFileFuncAsync = std::function<void(int, FileOperateCallbackFunc, void *)>;
    using RestoreFileFunc = std::function<bool(ProgressCallbackFunc, void *)>;
    using RestoreFileFuncAsync = std::function<void(ProgressCallbackFunc, void *, int, FileOperateCallbackFunc, void *)>;

    using TouchFileFunc = std::function<bool()>;
    using TouchFileFuncAsync = std::function<void(int, FileOperateCallbackFunc, void *)>;
    using MakeDirectoryFunc = std::function<bool()>;
    using MakeDirectoryFuncAsync = std::function<void(int, FileOperateCallbackFunc, void *)>;
    using CreateLinkFunc = std::function<bool(const QUrl &)>;
    using CreateLinkFuncAsync = std::function<void(const QUrl &, int, FileOperateCallbackFunc, void *)>;

    using SetFileInfoFunc = std::function<bool(const DFileInfo &)>;

    using CancelFunc = std::function<bool()>;
    using LastErrorFunc = std::function<DFMIOError()>;

public:
    DOperator(const QUrl &uri);
    virtual ~DOperator();

    QUrl uri() const;

    DFM_VIRTUAL bool renameFile(const QString &newName);
    DFM_VIRTUAL bool renameFile(const QUrl &toUrl);
    DFM_VIRTUAL bool copyFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    DFM_VIRTUAL bool moveFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    // async
    DFM_VIRTUAL void renameFileAsync(const QString &newName, int ioPriority = 0, FileOperateCallbackFunc func = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void copyFileAsync(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc progressfunc = nullptr, void *progressCallbackData = nullptr,
                                   int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void moveFileAsync(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                                   int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    DFM_VIRTUAL QString trashFile();
    DFM_VIRTUAL bool deleteFile();
    DFM_VIRTUAL bool restoreFile(ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    // async
    DFM_VIRTUAL void trashFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void deleteFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void restoreFileAsync(ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                                      int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    DFM_VIRTUAL bool touchFile();
    DFM_VIRTUAL bool makeDirectory();
    DFM_VIRTUAL bool createLink(const QUrl &link);
    // async
    DFM_VIRTUAL void touchFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void makeDirectoryAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    DFM_VIRTUAL void createLinkAsync(const QUrl &link, int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    DFM_VIRTUAL bool setFileInfo(const DFileInfo &fileInfo);

    DFM_VIRTUAL bool cancel();
    DFM_VIRTUAL DFMIOError lastError() const;

    //register
    void registerRenameFile(const RenameFileFunc &func);
    void registerRenameFileByStd(const RenameFileByStdFunc &func);
    void registerCopyFile(const CopyFileFunc &func);
    void registerMoveFile(const MoveFileFunc &func);
    void registerRenameFileAsync(const RenameFileFuncAsync &func);
    void registerCopyFileAsync(const CopyFileFuncAsync &func);
    void registerMoveFileAsync(const MoveFileFuncAsync &func);

    void registerTrashFile(const TrashFileFunc &func);
    void registerDeleteFile(const DeleteFileFunc &func);
    void registerRestoreFile(const RestoreFileFunc &func);
    void registerTrashFileAsync(const TrashFileFuncAsync &func);
    void registerDeleteFileAsync(const DeleteFileFuncAsync &func);
    void registerRestoreFileAsync(const RestoreFileFuncAsync &func);

    void registerTouchFile(const TouchFileFunc &func);
    void registerMakeDirectory(const MakeDirectoryFunc &func);
    void registerCreateLink(const CreateLinkFunc &func);
    void registerTouchFileAsync(const TouchFileFuncAsync &func);
    void registerMakeDirectoryAsync(const MakeDirectoryFuncAsync &func);
    void registerCreateLinkAsync(const CreateLinkFuncAsync &func);

    void registerSetFileInfo(const SetFileInfoFunc &func);

    void registerCancel(const CancelFunc &func);
    void registerLastError(const LastErrorFunc &func);

private:
    QSharedPointer<DOperatorPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DOPERATOR_H
