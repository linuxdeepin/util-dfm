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

    bool open(DFile::OpenFlag mode) DFM_OVERRIDE;
    bool close() DFM_OVERRIDE;
    qint64 read(char *data, qint64 maxSize) DFM_OVERRIDE;
    QByteArray read(qint64 maxSize) DFM_OVERRIDE;
    QByteArray readAll() DFM_OVERRIDE;
    qint64 write(const char *data, qint64 len) DFM_OVERRIDE;
    qint64 write(const char *data) DFM_OVERRIDE;
    qint64 write(const QByteArray &byteArray) DFM_OVERRIDE;
    bool seek(qint64 pos, DFile::DFMSeekType type = DFMSeekType::BEGIN) DFM_OVERRIDE;
    qint64 pos() DFM_OVERRIDE;
    bool flush() DFM_OVERRIDE;
    qint64 size() DFM_OVERRIDE;
    bool exists() DFM_OVERRIDE;
    uint16_t permissions(Permission permission = Permission::NoPermission) DFM_OVERRIDE;
    bool setPermissions(const uint16_t mode) DFM_OVERRIDE;
    DFMIOError lastError() const DFM_OVERRIDE;

private:
    QSharedPointer<DLocalFilePrivate> d = nullptr;
};

END_IO_NAMESPACE

#endif   // DLOCALFILE_H
