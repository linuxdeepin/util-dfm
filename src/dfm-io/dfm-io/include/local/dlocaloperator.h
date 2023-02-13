// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DLOCALOPERATOR_H
#define DLOCALOPERATOR_H

#include "dfmio_global.h"

#include "core/doperator.h"

BEGIN_IO_NAMESPACE

class DLocalOperatorPrivate;

class DLocalOperator : public DOperator
{
public:
    explicit DLocalOperator(const QUrl &uri);
    ~DLocalOperator();

    bool renameFile(const QString &newName) DFM_OVERRIDE;
    bool renameFile(const QUrl &toUrl) DFM_OVERRIDE;
    bool copyFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr) DFM_OVERRIDE;
    bool moveFile(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr) DFM_OVERRIDE;
    // async
    void renameFileAsync(const QString &newName, int ioPriority = 0, FileOperateCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void copyFileAsync(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void moveFileAsync(const QUrl &destUri, DFile::CopyFlag flag, ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                       int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;

    QString trashFile() DFM_OVERRIDE;
    bool deleteFile() DFM_OVERRIDE;
    bool restoreFile(ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr) DFM_OVERRIDE;
    // async
    void trashFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void deleteFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void restoreFileAsync(ProgressCallbackFunc func = nullptr, void *progressCallbackData = nullptr,
                          int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;

    bool touchFile() DFM_OVERRIDE;
    bool makeDirectory() DFM_OVERRIDE;
    bool createLink(const QUrl &link) DFM_OVERRIDE;
    // async
    void touchFileAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void makeDirectoryAsync(int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void createLinkAsync(const QUrl &link, int ioPriority = 0, FileOperateCallbackFunc operatefunc = nullptr, void *userData = nullptr) DFM_OVERRIDE;

    bool setFileInfo(const DFileInfo &fileInfo) DFM_OVERRIDE;

    bool cancel() DFM_OVERRIDE;
    DFMIOError lastError() const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalOperatorPrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALOPERATOR_H
