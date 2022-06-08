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

#ifndef DLOCALFILE_P_H
#define DLOCALFILE_P_H

#include "dfmio_global.h"

#include "core/dfile_p.h"

#include "gio/gio.h"

BEGIN_IO_NAMESPACE

class DLocalFile;

class DLocalFilePrivate : public QObject
{
public:
    explicit DLocalFilePrivate(DLocalFile *q);
    ~DLocalFilePrivate();

    bool open(DFile::OpenFlags mode);
    bool close();
    qint64 read(char *data, qint64 maxSize);
    QByteArray read(qint64 maxSize);
    QByteArray readAll();
    // async
    void readAsync(char *data, qint64 maxSize, int ioPriority = 0, DFile::ReadCallbackFunc func = nullptr, void *userData = nullptr);
    void readQAsync(qint64 maxSize, int ioPriority = 0, DFile::ReadQCallbackFunc func = nullptr, void *userData = nullptr);
    void readAllAsync(int ioPriority = 0, DFile::ReadAllCallbackFunc func = nullptr, void *userData = nullptr);

    qint64 write(const char *data, qint64 len);
    qint64 write(const char *data);
    qint64 write(const QByteArray &data);
    // async
    void writeAsync(const char *data, qint64 len, int ioPriority = 0, DFile::WriteCallbackFunc func = nullptr, void *userData = nullptr);
    void writeAllAsync(const char *data, int ioPriority = 0, DFile::WriteAllCallbackFunc func = nullptr, void *userData = nullptr);
    void writeQAsync(const QByteArray &byteArray, int ioPriority = 0, DFile::WriteQCallbackFunc func = nullptr, void *userData = nullptr);

    bool seek(qint64 pos, DFile::SeekType type = DFile::SeekType::kBegin);
    qint64 pos();
    bool flush();
    qint64 size();
    bool exists();
    DFile::Permissions permissions();
    bool setPermissions(DFile::Permissions permission);

    GInputStream *inputStream();
    GOutputStream *outputStream();

    DFMIOError lastError();
    void setError(DFMIOError error);
    void setErrorFromGError(GError *gerror);

    bool checkOpenFlags(DFile::OpenFlags *mode);

    void freeCancellable(GCancellable *gcancellable);

public:
    GIOStream *ioStream = nullptr;
    GInputStream *iStream = nullptr;
    GOutputStream *oStream = nullptr;

    DFMIOError error;

    DLocalFile *q = nullptr;
    GCancellable *gcancellable = nullptr;
    QByteArray readAllAsyncRet;
};

END_IO_NAMESPACE

#endif   // DLOCALFILE_P_H
