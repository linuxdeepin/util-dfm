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

class DLocalFilePrivate
{
    Q_DECLARE_PUBLIC(DLocalFile)
public:
    explicit DLocalFilePrivate(DLocalFile *ptr);
    ~DLocalFilePrivate();

    bool open(DFile::OpenFlag mode);
    bool close();
    qint64 read(char *data, qint64 maxSize);
    QByteArray read(qint64 maxSize);
    QByteArray readAll();
    qint64 write(const char *data, qint64 len);
    qint64 write(const char *data);
    qint64 write(const QByteArray &data);
    bool seek(qint64 pos);

public:
    GInputStream *input_stream = nullptr;
    GOutputStream *output_stream = nullptr;

    DLocalFile *q_ptr;
};

END_IO_NAMESPACE

#endif // DLOCALFILE_P_H
