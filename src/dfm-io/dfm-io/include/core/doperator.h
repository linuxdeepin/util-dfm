/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#ifndef DOPERATOR_H
#define DOPERATOR_H

#include "dfmio_global.h"
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
    enum class CopyFlag : uint16_t {
        None = 0,
        Overwrite = 1,
        Backup = 2,
        NoFollowSymlinks = 3,
        AllMetadata = 4,
        NofallbackForMove = 5,
        TargetDefaultPerms = 6,

        UserFlag = 0x100
    };

    using ProgressCallbackfunc = void (*)(int64_t, int64_t, void *);   // current_num_bytes, total_num_bytes, user_data

    using RenameFileFunc = std::function<bool(const QString &)>;
    using CopyFileFunc = std::function<bool(const QUrl &, CopyFlag, ProgressCallbackfunc, void *)>;
    using MoveFileFunc = std::function<bool(const QUrl &, CopyFlag, ProgressCallbackfunc, void *)>;

    using TrashFileFunc = std::function<bool()>;
    using DeleteFileFunc = std::function<bool()>;
    using RestoreFileFunc = std::function<bool(ProgressCallbackfunc, void *)>;

    using TouchFileFunc = std::function<bool()>;
    using MakeDirectoryFunc = std::function<bool()>;
    using CreateLinkFunc = std::function<bool(const QUrl &)>;
    using SetFileInfoFunc = std::function<bool(const DFileInfo &)>;

    using CancelFunc = std::function<bool()>;
    using LastErrorFunc = std::function<DFMIOError()>;

public:
    DOperator(const QUrl &uri);
    virtual ~DOperator();

    QUrl uri() const;

    DFM_VIRTUAL bool renameFile(const QString &newName);
    DFM_VIRTUAL bool copyFile(const QUrl &destUri, CopyFlag flag, ProgressCallbackfunc func = nullptr, void *userData = nullptr);
    DFM_VIRTUAL bool moveFile(const QUrl &destUri, CopyFlag flag, ProgressCallbackfunc func = nullptr, void *userData = nullptr);

    DFM_VIRTUAL bool trashFile();
    DFM_VIRTUAL bool deleteFile();
    DFM_VIRTUAL bool restoreFile(ProgressCallbackfunc func = nullptr, void *userData = nullptr);

    DFM_VIRTUAL bool touchFile();
    DFM_VIRTUAL bool makeDirectory();
    DFM_VIRTUAL bool createLink(const QUrl &link);
    DFM_VIRTUAL bool setFileInfo(const DFileInfo &fileInfo);

    DFM_VIRTUAL bool cancel();
    DFM_VIRTUAL DFMIOError lastError() const;

    //register
    void registerRenameFile(const RenameFileFunc &func);
    void registerCopyFile(const CopyFileFunc &func);
    void registerMoveFile(const MoveFileFunc &func);

    void registerTrashFile(const TrashFileFunc &func);
    void registerDeleteFile(const DeleteFileFunc &func);
    void registerRestoreFile(const RestoreFileFunc &func);

    void registerTouchFile(const TouchFileFunc &func);
    void registerMakeDirectory(const MakeDirectoryFunc &func);
    void registerCreateLink(const CreateLinkFunc &func);
    void registerSetFileInfo(const SetFileInfoFunc &func);

    void registerCancel(const CancelFunc &func);
    void registerLastError(const LastErrorFunc &func);

private:
    QSharedPointer<DOperatorPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DOPERATOR_H
