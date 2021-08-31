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

    DFM_VIRTUAL bool open(DFile::OpenFlag mode);
    DFM_VIRTUAL bool close();
    DFM_VIRTUAL qint64 read(char *data, qint64 maxSize);
    DFM_VIRTUAL QByteArray read(qint64 maxSize);
    DFM_VIRTUAL QByteArray readAll();
    DFM_VIRTUAL qint64 write(const char *data, qint64 len);
    DFM_VIRTUAL qint64 write(const char *data);
    DFM_VIRTUAL qint64 write(const QByteArray &byteArray);
    DFM_VIRTUAL bool seek(qint64 pos, DFile::DFMSeekType type = DFMSeekType::BEGIN);
    DFM_VIRTUAL qint64 pos();
    DFM_VIRTUAL bool flush();
    DFM_VIRTUAL qint64 size();
    DFM_VIRTUAL bool exists();

private:
    QSharedPointer<DLocalFilePrivate> d_ptr;
    Q_DECLARE_PRIVATE(DLocalFile)
};

END_IO_NAMESPACE

#endif // DLOCALFILE_H
