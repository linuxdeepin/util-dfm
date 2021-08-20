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
#ifndef DFILE_P_H
#define DFILE_P_H

#include "core/dfile.h"
#include "error/error.h"

BEGIN_IO_NAMESPACE

class DFile;

class DFilePrivate
{
    Q_DECLARE_PUBLIC(DFile)
public:
    explicit DFilePrivate(DFile *q);
    virtual ~DFilePrivate();

    /*virtual bool openFile(DFile::OpenFlag mode) = 0;
    virtual bool closeFile() = 0;
    virtual qint64 readData(char *data, qint64 maxSize) = 0;
    virtual qint64 writeData(const char *data, qint64 len) = 0;
    virtual bool seekFile(qint64 pos) = 0;*/

public:
    DFile *q_ptr = nullptr;
    QUrl uri;

    bool isOpen = false;
    DFMIOError error;

    DFile::OpenFunc openFunc = nullptr;
    DFile::CloseFunc closeFunc = nullptr;
    DFile::ReadFunc readFunc = nullptr;
    DFile::ReadQFunc readQFunc = nullptr;
    DFile::ReadAllFunc readAllFunc = nullptr;
    DFile::WriteFunc writeFunc = nullptr;
    DFile::WriteAllFunc writeAllFunc = nullptr;
    DFile::WriteQFunc writeQFunc = nullptr;
    DFile::SeekFunc seekFunc = nullptr;
};

END_IO_NAMESPACE

#endif // DFILE_P_H
