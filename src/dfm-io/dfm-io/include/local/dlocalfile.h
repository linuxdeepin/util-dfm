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

#ifndef DLOCALFILE_H
#define DLOCALFILE_H

#include "dfmio_global.h"

#include "core/dfile.h"

BEGIN_IO_NAMESPACE

class DLocalFilePrivate;

class DLocalFile : public DFile
{
public:
    explicit DLocalFile(const QUrl &uri);
    virtual ~DLocalFile();

    bool open(DFile::OpenFlags mode) DFM_OVERRIDE;
    bool close() DFM_OVERRIDE;

    qint64 read(char *data, qint64 maxSize) DFM_OVERRIDE;
    QByteArray read(qint64 maxSize) DFM_OVERRIDE;
    QByteArray readAll() DFM_OVERRIDE;
    // async
    void readAsync(char *data, qint64 maxSize, int ioPriority = 0, ReadCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void readQAsync(qint64 maxSize, int ioPriority = 0, ReadQCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void readAllAsync(int ioPriority = 0, ReadAllCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;

    qint64 write(const char *data, qint64 len) DFM_OVERRIDE;
    qint64 write(const char *data) DFM_OVERRIDE;
    qint64 write(const QByteArray &byteArray) DFM_OVERRIDE;
    // async
    void writeAsync(const char *data, qint64 len, int ioPriority = 0, WriteCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void writeAllAsync(const char *data, int ioPriority = 0, WriteAllCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;
    void writeQAsync(const QByteArray &byteArray, int ioPriority = 0, WriteQCallbackFunc func = nullptr, void *userData = nullptr) DFM_OVERRIDE;

    bool seek(qint64 pos, DFile::SeekType type = SeekType::kBegin) DFM_OVERRIDE;
    qint64 pos() DFM_OVERRIDE;
    bool flush() DFM_OVERRIDE;
    qint64 size() DFM_OVERRIDE;
    bool exists() DFM_OVERRIDE;
    Permissions permissions() const DFM_OVERRIDE;
    bool setPermissions(Permissions permission) DFM_OVERRIDE;

    void setError(DFMIOError error);
    DFMIOError lastError() const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalFilePrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALFILE_H
