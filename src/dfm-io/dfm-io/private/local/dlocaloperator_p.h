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

#ifndef DLOCALOPERATOR_P_H
#define DLOCALOPERATOR_P_H

#include "dfmio_global.h"
#include "core/doperator_p.h"

#include "gio/gio.h"

BEGIN_IO_NAMESPACE

class DLocalOperator;

class DLocalOperatorPrivate
{
public:
    explicit DLocalOperatorPrivate(DLocalOperator *q);
    ~DLocalOperatorPrivate();

    bool renameFile(const QString &new_name);
    bool renameFile(const QUrl &toUrl);
    bool copyFile(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    bool moveFile(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    void renameFileAsync(const QString &newName, int ioPriority = 0, DOperator::FileOperateCallbackFunc func = nullptr, void *userData = nullptr);
    void copyFileAsync(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void moveFileAsync(const QUrl &dstUri, DFile::CopyFlag flag, DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    bool trashFile();
    bool deleteFile();
    bool restoreFile(DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr);
    void trashFileAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void deleteFileAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void restoreFileAsync(DOperator::ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                          int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    bool touchFile();
    bool makeDirectory();
    bool createLink(const QUrl &link);
    void touchFileAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void makeDirectoryAsync(int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);
    void createLinkAsync(const QUrl &link, int ioPriority = 0, DOperator::FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr);

    bool setFileInfo(const DFileInfo &fileInfo);
    bool cancel();

    DFMIOError lastError();
    void setErrorFromGError(GError *gerror);

    GFile *makeGFile(const QUrl &url);
    void freeCancellable(GCancellable *gcancellable);

private:
    DLocalOperator *q = nullptr;
    GCancellable *gcancellable = nullptr;

    DFMIOError error;
};

END_IO_NAMESPACE

#endif   // DLOCALOPERATOR_P_H
